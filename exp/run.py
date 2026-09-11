#!/usr/bin/env python3
"""Single entry point for every experiment in the paper."""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent

SUITES = {
    "engineering_set_main": [
        "sec_5_2_1_hhh_optimizations", "fig_5_1_hhh_epsilon", "fig_5_2_hhh_partitions",
        "fig_5_3_egst_partitions", "fig_5_4_static_baselines",
        "fig_5_5_dynamic_baselines",
    ],
    "engineering_set_appendix": [
        "fig_a_1_extra_aux", "fig_a_2_rebalance_factor",
        "fig_a_3_egst_optimizations", "fig_a_4_hhh_single_patterns",
        "fig_a_5_egst_single_patterns",
    ],
    "full_set": ["fig_5_6_full_set"],
}
EXPERIMENTS = tuple(experiment for names in SUITES.values() for experiment in names)
PLOT_PROGRAMS = {
    "sec_5_2_1_hhh_optimizations": "hhh_any_anchors/plot.py",
    "fig_5_1_hhh_epsilon": "epstab/plot_eps.py",
    "fig_5_2_hhh_partitions": "epstab/plot_partitions.py",
    "fig_5_3_egst_partitions": "epstab/plot_partitions.py",
    "fig_5_4_static_baselines": "compare/plot_150k_combined.py",
    "fig_5_5_dynamic_baselines": "csm/plot.py",
    "fig_5_6_full_set": "compare/plot_comp_all_graphs.py",
    "fig_a_1_extra_aux": "arrows/plot.py",
    "fig_a_2_rebalance_factor": "epstab/plot_recomp_factor_scatter.py",
    "fig_a_3_egst_optimizations": "egst/plot_high.py",
    "fig_a_4_hhh_single_patterns": "epstab/plot_partitions.py",
    "fig_a_5_egst_single_patterns": "epstab/plot_partitions.py",
}

# Runtime projections based on previous per-run measurements, including a 20%
# contingency. These are benchmark CPU-hours, not guarantees; see the README.
ESTIMATED_CPU_HOURS = {
    "sec_5_2_1_hhh_optimizations": 158.373,
    "fig_5_1_hhh_epsilon": 20.719,
    "fig_5_2_hhh_partitions": 119.532,
    "fig_5_3_egst_partitions": 131.935,
    "fig_5_4_static_baselines": 94.325,
    "fig_5_5_dynamic_baselines": 899.819,
    "fig_5_6_full_set": 1480.034,
    "fig_a_1_extra_aux": 9.165,
    "fig_a_2_rebalance_factor": 37.604,
    "fig_a_3_egst_optimizations": 70.651,
    "fig_a_4_hhh_single_patterns": 189.312,
    "fig_a_5_egst_single_patterns": 144.827,
}

# Memory-aware list-scheduling model for the target machine: 16 workers,
# 1.48 TB decimal RAM, two equal NUMA nodes, and eight workers per node.
ESTIMATED_WALL_HOURS_16_1_48TB = {
    "sec_5_2_1_hhh_optimizations": 49.102,
    "fig_5_1_hhh_epsilon": 1.381,
    "fig_5_2_hhh_partitions": 7.536,
    "fig_5_3_egst_partitions": 8.347,
    "fig_5_4_static_baselines": 6.767,
    # Add separately list-scheduled DNA time (16 workers, including tail).
    "fig_5_5_dynamic_baselines": 61.127,
    "fig_5_6_full_set": 107.937,
    "fig_a_1_extra_aux": 0.648,
    "fig_a_2_rebalance_factor": 2.386,
    "fig_a_3_egst_optimizations": 4.837,
    "fig_a_4_hhh_single_patterns": 11.854,
    "fig_a_5_egst_single_patterns": 9.048,
}


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Reproduce experiments from Fully Dynamic Triangle and "
                    "4-Vertex Subgraph Counting: From Theory to Practice and Back Again")
    p.add_argument("--cpus", help="CPU set, e.g. 0,8,16-18. Default: one allowed CPU per L3 cache.")
    group = p.add_mutually_exclusive_group()
    group.add_argument("--engineering_set_main", action="store_true", help="run Figures 5.1--5.5")
    group.add_argument("--engineering_set_appendix", action="store_true", help="run Figures A.1--A.5")
    group.add_argument("--full_set", action="store_true", help="run Figure 5.6 on all 56 graphs")
    group.add_argument("--all", action="store_true", help="run all paper experiments")
    group.add_argument("--experiment", choices=EXPERIMENTS, metavar="NAME",
                       help="run one experiment (use --list to show names)")
    p.add_argument("--data-only", action="store_true", help="download and prepare data, then stop")
    p.add_argument("--toy", action="store_true",
                   help="run main-suite experiments on the first 2,000 updates")
    p.add_argument("--skip-data", action="store_true", help="do not invoke data setup")
    p.add_argument("--skip-plots", action="store_true", help="retain CSV results without plotting")
    p.add_argument("--force", action="store_true", help="rerun completed jobs")
    p.add_argument("--dry-run", action="store_true", help="validate and print work without executing jobs")
    p.add_argument("--list", action="store_true", help="list suites, experiments, and detected CPUs")
    p.add_argument("--estimate", action="store_true", help="print estimated runtimes and stop")
    p.add_argument("--check-resources", action="store_true", help=argparse.SUPPRESS)
    p.add_argument("--data-dir", type=Path)
    p.add_argument("--results-dir", type=Path)
    p.add_argument("--plots-dir", type=Path)
    return p


def detect_cpus() -> str:
    """Choose one schedulable logical CPU from every distinct L3 cache."""
    allowed = set(os.sched_getaffinity(0))
    caches: dict[str, int] = {}
    cache_root = Path("/sys/devices/system/cpu")
    for cpu in sorted(allowed):
        shared = cache_root / f"cpu{cpu}" / "cache" / "index3" / "shared_cpu_list"
        try:
            key = shared.read_text().strip()
        except OSError:
            key = f"cpu:{cpu}"
        caches.setdefault(key, cpu)
    return ",".join(map(str, caches.values() or sorted(allowed)))


def validate_cpus(value: str) -> str:
    selected: set[int] = set()
    try:
        for item in value.split(","):
            bounds = item.strip().split("-", 1)
            if not bounds[0]:
                raise ValueError
            selected.update([int(bounds[0])] if len(bounds) == 1 else
                            range(int(bounds[0]), int(bounds[1]) + 1))
    except ValueError as exc:
        raise SystemExit(f"invalid --cpus value: {value!r}") from exc
    allowed = set(os.sched_getaffinity(0))
    if not selected or not selected <= allowed:
        raise SystemExit(f"--cpus contains unavailable CPUs; selected={sorted(selected)}, allowed={sorted(allowed)}")
    return ",".join(map(str, sorted(selected)))


def selected(args: argparse.Namespace) -> tuple[list[str], list[str]]:
    if args.experiment:
        suite = next(suite for suite, names in SUITES.items() if args.experiment in names)
        suites = [suite]
        experiments = [args.experiment]
        return suites, experiments
    if args.engineering_set_main:
        suites = ["engineering_set_main"]
    elif args.engineering_set_appendix:
        suites = ["engineering_set_appendix"]
    elif args.full_set:
        suites = ["full_set"]
    else:  # --all and no selector both mean the complete reproduction.
        suites = list(SUITES)
    experiments = [e for suite in suites for e in SUITES[suite]]
    return suites, experiments


def duration(hours: float) -> str:
    if hours >= 24:
        return f"{hours / 24:.1f} days"
    if hours >= 1:
        return f"{hours:.1f} h"
    return f"{hours * 60:.0f} min"


def print_estimate(experiments: list[str], cpu_count: int) -> None:
    memory_model = cpu_count == 16
    if memory_model:
        print("Estimated runtime based on previous results (16 workers, 1.48 TB RAM, "
              "2 NUMA nodes, 8 workers/node):")
    else:
        print(f"Estimated runtime based on previous results ({cpu_count} concurrent CPUs):")
    total = 0.0
    total_wall = 0.0
    for experiment in experiments:
        cpu_hours = ESTIMATED_CPU_HOURS[experiment]
        total += cpu_hours
        wall = (ESTIMATED_WALL_HOURS_16_1_48TB[experiment] if memory_model
                else cpu_hours / cpu_count)
        total_wall += wall
        print(f"  {experiment:<36} {cpu_hours:>8.1f} CPU-h  ~ {duration(wall):>9}")
    print(f"  {'TOTAL':<36} {total:>8.1f} CPU-h  ~ {duration(total_wall):>9}")
    print("Estimate includes a 20% contingency but excludes image build, downloads, "
          "preprocessing, plotting, and other unmodeled I/O/system effects.")


def check_resources(experiments: list[str]) -> None:
    """Validate only immutable inputs needed by the selected experiments."""
    from scripts.run_experiment import EXP_ROOT, jobs_for, memory_requirements_kib

    common_plot_files = [HERE / "plotting/util.py", HERE / "plotting/config.yaml"]
    missing_plot_files = [str(path) for path in common_plot_files if not path.is_file()]
    for experiment in experiments:
        plot_program = HERE / "plotting" / PLOT_PROGRAMS[experiment]
        if not plot_program.is_file():
            missing_plot_files.append(str(plot_program))
        jobs = jobs_for(experiment)
        memory_requirements_kib(experiment, jobs)
        missing_queries = sorted({
            str(EXP_ROOT / "resources/csm/pattern" / f"{job.pattern}.q")
            for job in jobs
            if job.reference and job.reference != "dna"
            and not (EXP_ROOT / "resources/csm/pattern" / f"{job.pattern}.q").is_file()
        })
        if missing_queries:
            raise SystemExit(
                f"missing reference query files for {experiment}: " + ", ".join(missing_queries)
            )
    if missing_plot_files:
        raise SystemExit("missing plotting resources: " + ", ".join(sorted(set(missing_plot_files))))
    print(f"Resources ready for {len(experiments)} experiment(s).")


def main() -> int:
    args = parser().parse_args()
    cpus = validate_cpus(args.cpus or detect_cpus())
    suites, experiments = selected(args)
    if args.toy and suites != ["engineering_set_main"]:
        raise SystemExit(
            "--toy is available with --engineering_set_main or with a main-suite --experiment"
        )
    args.data_dir = args.data_dir or HERE / "data"
    args.results_dir = args.results_dir or HERE / "results" / ("toy" if args.toy else "")
    args.plots_dir = args.plots_dir or HERE / "plots" / ("toy" if args.toy else "")
    if args.list:
        print(f"CPUs: {cpus}")
        for suite, names in SUITES.items():
            print(f"{suite}:")
            for name in names:
                print(f"  {name}")
        return 0
    if args.estimate:
        if args.toy:
            print("Toy runtime is not estimated from the full-size paper measurements.")
        else:
            print_estimate(experiments, len(cpus.split(",")))
        return 0
    if args.check_resources:
        if not args.data_only:
            check_resources(experiments)
        return 0

    args.data_dir.mkdir(parents=True, exist_ok=True)
    args.results_dir.mkdir(parents=True, exist_ok=True)
    args.plots_dir.mkdir(parents=True, exist_ok=True)

    if args.toy:
        print("Toy mode: first 2,000 updates; runtime is not estimated from full-size results.")
    else:
        print_estimate(experiments, len(cpus.split(",")))

    if not args.skip_data:
        print("\n[stage 1/4] Get and prepare missing data", flush=True)
        command = [str(HERE / "scripts" / "get_data.sh"), "--data-dir", str(args.data_dir)]
        for suite in suites:
            command += ["--suite", suite]
        if args.toy:
            command.append("--toy")
        if args.dry_run:
            command.append("--dry-run")
        subprocess.run(command, check=True, cwd=ROOT)
    else:
        print("\n[stage 1/4] Data preparation skipped", flush=True)
    if args.data_only:
        return 0

    total = len(experiments)
    for number, experiment in enumerate(experiments, 1):
        print(f"\n[{number}/{total}] {experiment}", flush=True)
        command = [str(HERE / "experiments" / f"{experiment}.sh"),
                   "--cpus", cpus, "--data-dir", str(args.data_dir),
                   "--results-dir", str(args.results_dir), "--plots-dir", str(args.plots_dir)]
        if args.force:
            command.append("--force")
        if args.dry_run:
            command.append("--dry-run")
        if args.skip_plots:
            command.append("--skip-plots")
        if args.toy:
            command.append("--toy")
        subprocess.run(command, check=True, cwd=ROOT)
        print(f"[{number}/{total} done] {experiment}", flush=True)
    if args.dry_run:
        print("\nDry run completed; no data was downloaded and no experiments or plots were run.")
    else:
        print(f"\nAll requested experiments completed. Plots: {args.plots_dir}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        # The child command already printed the actionable error.
        raise SystemExit(exc.returncode) from None
