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


def write_partition_inputs(frame: pd.DataFrame, stage: Path) -> None:
    (stage / "epstab").mkdir(parents=True)
    (stage / "hindex").mkdir(parents=True)
    algo = frame["algo"].astype(str)
    partition = frame["partition"].astype(str)
    mode = pd.to_numeric(frame.get("recompute_mode", 0), errors="coerce").fillna(0)
    selections = {
        stage / "epstab/results_hhh_eps.csv": algo.isin(("hhh", "v_hhh")) & partition.eq("epstab") & mode.ne(3),
        stage / "epstab/results_eps_late.csv": algo.isin(("hhh", "v_hhh")) & partition.eq("epstab") & mode.eq(3),
        stage / "hindex/results_hhh.csv": algo.isin(("hhh", "v_hhh")) & partition.eq("hindex"),
        stage / "epstab/results_eps_egst.csv": algo.eq("egst") & partition.eq("epstab"),
        stage / "hindex/results_egst.csv": algo.eq("egst") & partition.eq("hindex"),
    }
    for path, mask in selections.items():
        frame.loc[mask].to_csv(path, index=False)


def plot(experiment: str, results: Path, output: Path, toy: bool = False) -> None:
    frame = pd.read_csv(results)
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
                for pattern in PAPER_PATTERNS:
                    publish(work / f"plots/PARTS_{selected_algo}_{pattern}.pdf", output, f"{experiment}_{pattern}.pdf")

        elif experiment == "fig_5_4_static_baselines":
            work = stage / "compare"; work.mkdir()
            static = frame.copy()
            epsilon = pd.to_numeric(static.get("epsilon", -1), errors="coerce")
            gradual = pd.to_numeric(static.get("gradual_factor", -1), errors="coerce")
            mode = pd.to_numeric(static.get("recompute_mode", 0), errors="coerce").fillna(0)
            direct = pd.to_numeric(static.get("directVertexChange", 0), errors="coerce").fillna(0)
            high = pd.to_numeric(static.get("s3HighAnchors", 0), errors="coerce").fillna(0)
            static.loc[static["algo"].eq("ob"), ["algo", "partition"]] = "oba", "none"
            static.loc[static["algo"].eq("escape"), "partition"] = "none"
            static.loc[static["algo"].eq("v_hhh") & epsilon.round(3).eq(.417), "partition"] = "epstab_0417"
            static.loc[static["algo"].eq("hhh") & mode.eq(3), "partition"] = "epstab_late_02"
            static.loc[static["algo"].eq("egst") & gradual.round(4).eq(2), "partition"] = "hindex2.0"
            optimized_egst = static["algo"].eq("egst") & direct.eq(1) & high.eq(1)
            static.loc[optimized_egst, ["algo", "partition"]] = "egst_high_direct", "hindex1.0625"
            static.to_csv(work / "results_150k.csv", index=False)
            details = results.parent / "details"
            shutil.copytree(details, work / "resultsFR") if details.is_dir() else (work / "resultsFR").mkdir()
            run_original(PLOTTING / "compare/plot_150k_combined.py", work)
            publish(work / "plots/static_compare.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_5_5_dynamic_baselines":
            work = stage / "csm"; work.mkdir()
            ours = frame.copy()
            csm = pd.read_csv(results.with_name("results_csm.csv"))
            rapid = pd.read_csv(results.with_name("results_rapidflow.csv"))
            structure = ours.filter(regex=r"^structure_.*$")
            subgraph = ours.filter(regex=r"^subgraph_.*$")
            ours["aux_time"] = structure.apply(pd.to_numeric, errors="coerce").sum(axis=1)
            ours["count_time"] = subgraph.apply(pd.to_numeric, errors="coerce").sum(axis=1)
            mode = pd.to_numeric(ours.get("recompute_mode", 0), errors="coerce").fillna(0)
            epsilon = pd.to_numeric(ours.get("epsilon", -1), errors="coerce")
            gradual = pd.to_numeric(ours.get("gradual_factor", -1), errors="coerce")
            late = ours["partition"].eq("epstab") & mode.eq(3)
            ours.loc[late, "partition"] = "epstab_late_" + epsilon[late].map(lambda value: f"{value:g}")
            indexed = ours["partition"].eq("hindex")
            ours.loc[indexed, "partition"] = "hindex" + gradual[indexed].map(lambda value: f"{value:g}")
            ours.loc[ours["algo"].eq("egst"), "algo"] = "egst_high_direct"
            csm["time"] = pd.to_numeric(csm.get("time"), errors="coerce") * 1000
            ours.to_csv(work / "results_ours.csv", index=False)
            csm.to_csv(work / "results_baseline.csv", index=False)
            rapid.to_csv(work / "results_baseline_rapid.csv", index=False)
            dna = pd.read_csv(results.with_name("results_dna.csv"))
            if toy:
                dna["graph"] = dna["graph"].str.replace(r"_2k(?=\.e$|$)", "_2m", regex=True)
            dna.to_csv(work / "results_baseline_dna.csv", index=False)
            run_original(PLOTTING / "csm/plot.py", work)
            publish(work / "plots/comp_to_dyn_baselines.pdf", output, f"{experiment}.pdf")

        elif experiment == "fig_5_6_full_set":
            work = stage / "compare"; work.mkdir()
            frame.to_csv(work / "results_full_all.csv", index=False)
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
    args = parser.parse_args()
    plot(args.experiment, args.results, args.output, args.toy)


if __name__ == "__main__":
    main()
