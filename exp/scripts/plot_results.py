#!/usr/bin/env python3
"""Run the paper's original plotting programs on freshly collected results."""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import pandas as pd

EXP_ROOT = Path(__file__).resolve().parents[1]
PLOTTING = EXP_ROOT / "plotting"
PAPER_PATTERNS = ("tCycles", "tPaths", "paws", "fCycles", "diamonds", "fCliques")


def run_original(script: Path, cwd: Path) -> None:
    # Vendored scripts may assume this directory already exists (in particular
    # the partition and CSM plots). Every invocation uses a fresh workspace.
    (cwd / "plots").mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment.update({"MPLBACKEND": "Agg", "PYTHONUNBUFFERED": "1"})
    completed = subprocess.run(
        [sys.executable, str(script)], cwd=cwd, env=environment,
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if completed.returncode:
        if completed.stdout:
            print(completed.stdout, file=sys.stderr, end="")
        raise SystemExit(f"plotting failed: {script.name}")


def publish(source: Path, output: Path, name: str | None = None) -> None:
    if not source.is_file():
        raise SystemExit(f"original plotting script did not create {source}")
    target = output / (name or source.name)
    shutil.copy2(source, target)
    print(f"Wrote {target}")


def numeric_column(frame: pd.DataFrame, name: str, default: float = float("nan")) -> pd.Series:
    """Return an index-aligned numeric series even for older CSV schemas."""
    return pd.to_numeric(frame.get(name, pd.Series(default, index=frame.index)), errors="coerce")


def write_partition_inputs(frame: pd.DataFrame, stage: Path) -> None:
    (stage / "epstab").mkdir(parents=True)
    (stage / "hindex").mkdir(parents=True)
    algo = frame["algo"].astype(str)
    partition = frame["partition"].astype(str)
    mode = numeric_column(frame, "recompute_mode", 0).fillna(0)
    selections = {
        stage / "epstab/results_hhh_eps.csv": algo.isin(("hhh", "v_hhh")) & partition.eq("epstab") & mode.ne(3),
        stage / "epstab/results_eps_late.csv": algo.isin(("hhh", "v_hhh")) & partition.eq("epstab") & mode.eq(3),
        stage / "hindex/results_hhh.csv": algo.isin(("hhh", "v_hhh")) & partition.eq("hindex"),
        stage / "epstab/results_eps_egst.csv": algo.eq("egst") & partition.eq("epstab"),
        stage / "hindex/results_egst.csv": algo.eq("egst") & partition.eq("hindex"),
    }
    for path, mask in selections.items():
        frame.loc[mask].to_csv(path, index=False)


def plot(experiment: str, results: Path, output: Path, toy: bool = False,
         no_references: bool = False, only_hhh: bool = False,
         pattern: str | None = None) -> None:
    frame = pd.read_csv(results)
    if pattern:
        paper_pattern = {"triangle": "tCycles", "tPath": "tPaths", "paw": "paws",
                         "fCycle": "fCycles", "diamond": "diamonds", "fClique": "fCliques"}[pattern]
        frame = frame[frame["pattern"].eq(paper_pattern)]
    if frame.empty:
        raise SystemExit(f"No collected result rows in {results}")
    if toy:
        # Original plotting programs use the dataset suffix as a graph key.
        # Alias only this temporary plotting frame; raw toy results retain _2k.
        frame["graph"] = frame["graph"].astype(str).str.replace(r"_2k$", "_2m", regex=True)
    output.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix=f"plot-{experiment}-") as temporary:
        stage = Path(temporary)

        if experiment == "sec_5_2_1_hhh_optimizations":
            work = stage / "hhh_any_anchors"; work.mkdir()
            frame.to_csv(work / "results.csv", index=False)
            run_original(PLOTTING / "hhh_any_anchors/plot.py", work)
            publish(work / "plots/hhh_all_comb_variants.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_5_1_hhh_epsilon":
            work = stage / "epstab"; work.mkdir()
            frame.to_csv(work / "results_hhh_eps.csv", index=False)
            run_original(PLOTTING / "epstab/plot_eps.py", work)
            publish(work / "plots_epstab/EPS_epstab_hhh_all_grid.pdf", output, f"{experiment}.pdf")

        elif experiment in {"fig_5_2_hhh_partitions", "fig_5_3_egst_partitions",
                            "fig_a_4_hhh_single_patterns", "fig_a_5_egst_single_patterns"}:
            write_partition_inputs(frame, stage)
            work = stage / "epstab"
            run_original(PLOTTING / "epstab/plot_partitions.py", work)
            if experiment == "fig_5_2_hhh_partitions":
                publish(work / "plots/PARTS_hhh_all.pdf", output, f"{experiment}.pdf")
            elif experiment == "fig_5_3_egst_partitions":
                publish(work / "plots/PARTS_egst_all.pdf", output, f"{experiment}.pdf")
            else:
                selected_algo = "hhh" if experiment == "fig_a_4_hhh_single_patterns" else "egst"
                for paper_pattern in ((paper_pattern,) if pattern else PAPER_PATTERNS):
                    publish(work / f"plots/PARTS_{selected_algo}_{paper_pattern}.pdf", output,
                            f"{experiment}_{paper_pattern}.pdf")

        elif experiment == "fig_5_4_static_baselines":
            work = stage / "compare"; work.mkdir()
            static = frame.copy()
            epsilon = numeric_column(static, "epsilon", -1)
            gradual = numeric_column(static, "gradual_factor", -1)
            mode = numeric_column(static, "recompute_mode", 0).fillna(0)
            direct = numeric_column(static, "directVertexChange", 0).fillna(0)
            high = numeric_column(static, "s3HighAnchors", 0).fillna(0)
            static.loc[static["algo"].eq("ob"), ["algo", "partition"]] = "oba", "none"
            static.loc[static["algo"].eq("escape"), "partition"] = "none"
            static.loc[static["algo"].eq("v_hhh") & epsilon.round(3).eq(.417), "partition"] = "epstab_0417"
            static.loc[static["algo"].eq("hhh") & mode.eq(3), "partition"] = "epstab_late_02"
            static.loc[static["algo"].eq("egst") & gradual.round(4).eq(2), "partition"] = "hindex2.0"
            optimized_egst = static["algo"].eq("egst") & direct.eq(1) & high.eq(1)
            static.loc[optimized_egst, ["algo", "partition"]] = "egst_high_direct", "hindex1.0625"
            if no_references:
                old = pd.read_csv(EXP_ROOT / "resources/historical/results_150k.csv")
                old = old.loc[old["algo"].isin(("oba", "escape"))]
                static = pd.concat((static, old), ignore_index=True)
            static.to_csv(work / "results_150k.csv", index=False)
            details = results.parent / "details"
            shutil.copytree(details, work / "resultsFR") if details.is_dir() else (work / "resultsFR").mkdir()
            if no_references:
                (work / "historical_references_without_inset_traces").touch()
            run_original(PLOTTING / "compare/plot_150k_combined.py", work)
            publish(work / "plots/static_compare.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_5_5_dynamic_baselines":
            work = stage / "csm"; work.mkdir()
            ours = frame.copy()
            if no_references:
                reference_path = EXP_ROOT / "resources/historical"
                required = [reference_path / name for name in
                            ("results_baseline.csv", "results_baseline_rapid.csv", "results_baseline_dna.csv")]
            else:
                reference_path = results.parent
                required = [reference_path / name for name in
                            ("results_csm.csv", "results_rapidflow.csv", "results_dna.csv")]
            missing = [str(path) for path in required if not path.is_file()]
            if missing:
                raise SystemExit("missing historical reference results: " + ", ".join(missing))
            csm = pd.read_csv(required[0])
            rapid = pd.read_csv(required[1])
            structure = ours.filter(regex=r"^structure_.*$")
            subgraph = ours.filter(regex=r"^subgraph_.*$")
            derived_aux = structure.apply(pd.to_numeric, errors="coerce").sum(axis=1, min_count=1)
            derived_count = subgraph.apply(pd.to_numeric, errors="coerce").sum(axis=1, min_count=1)
            blank = pd.Series(index=ours.index, dtype=float)
            ours["aux_time"] = derived_aux.combine_first(pd.to_numeric(ours.get("aux_time", blank), errors="coerce"))
            ours["count_time"] = derived_count.combine_first(pd.to_numeric(ours.get("count_time", blank), errors="coerce"))
            mode = numeric_column(ours, "recompute_mode", 0).fillna(0)
            epsilon = numeric_column(ours, "epsilon", -1)
            gradual = numeric_column(ours, "gradual_factor", -1)
            late = ours["partition"].eq("epstab") & mode.eq(3)
            ours.loc[late, "partition"] = epsilon[late].map(
                lambda value: f"epstab_late_{value:g}" if pd.notna(value) else "epstab")
            indexed = ours["partition"].eq("hindex")
            ours.loc[indexed, "partition"] = gradual[indexed].map(
                lambda value: f"hindex{value:g}" if pd.notna(value) else "hindex")
            ours.loc[ours["algo"].eq("egst"), "algo"] = "egst_high_direct"
            if not no_references and not only_hhh:
                csm["time"] = pd.to_numeric(csm.get("time"), errors="coerce") * 1000
            ours.to_csv(work / "results_ours.csv", index=False)
            csm.to_csv(work / "results_baseline.csv", index=False)
            rapid.to_csv(work / "results_baseline_rapid.csv", index=False)
            dna = pd.read_csv(required[2])
            if toy:
                dna["graph"] = dna["graph"].str.replace(r"_2k(?=\.e$|$)", "_2m", regex=True)
            dna.to_csv(work / "results_baseline_dna.csv", index=False)
            run_original(PLOTTING / "csm/plot.py", work)
            publish(work / "plots/comp_to_dyn_baselines.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_5_6_full_set":
            work = stage / "compare"; work.mkdir()
            full = frame.copy()
            indexed = full["partition"].eq("hindex")
            full["epsilon"] = numeric_column(full, "epsilon").astype(float)
            full.loc[indexed, "epsilon"] = numeric_column(full, "gradual_factor")[indexed]
            full.to_csv(work / "results_full_all.csv", index=False)
            run_original(PLOTTING / "compare/plot_comp_all_graphs.py", work)
            publish(work / "plots/all_comp.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_a_1_extra_aux":
            work = stage / "arrows"; work.mkdir()
            frame.to_csv(work / "results_orig_partition.csv", index=False)
            run_original(PLOTTING / "arrows/plot.py", work)
            publish(work / "plots/pattern_boxplot.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_a_2_rebalance_factor":
            work = stage / "epstab"; work.mkdir()
            frame.to_csv(work / "results_recomp_factor.csv", index=False)
            run_original(PLOTTING / "epstab/plot_recomp_factor_scatter.py", work)
            publish(work / "plots_RF/RFACTOR_IMPROVEMENT_hhh_0.2_all.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_a_3_egst_optimizations":
            work = stage / "egst"; work.mkdir()
            frame.to_csv(work / "results.csv", index=False)
            run_original(PLOTTING / "egst/plot_high.py", work)
            publish(work / "plots/all_all_stats_high.pdf", output, f"{experiment}.pdf")

        else:
            raise SystemExit(f"No paper plot mapping for {experiment}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("experiment")
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--toy", action="store_true")
    parser.add_argument("--no-references", action="store_true")
    parser.add_argument("--only-hhh", action="store_true")
    parser.add_argument("--pattern", choices=("triangle", "tPath", "paw", "fCycle", "diamond", "fClique"))
    args = parser.parse_args()
    plot(args.experiment, args.results, args.output, args.toy,
         args.no_references, args.only_hhh, args.pattern)


if __name__ == "__main__":
    main()
