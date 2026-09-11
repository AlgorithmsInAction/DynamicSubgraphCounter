import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys, os
import matplotlib.colors as mcolors
import matplotlib.patches as mpatches
import matplotlib.lines as mlines
import itertools

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import palette, algo_hatches, partition_display, algo_display, apply_graph_groups, geo_mean
from util import ComputerModernLogFormatter



def numbers_for_paper(df): 

    dfs =[]

    for metric in ["time", "max_update"]:
        pivot = df.pivot(index="graph", columns="algo", values=metric)
        result = pivot.copy()
        for p1, p2 in itertools.combinations(pivot.columns, 2):
            result[f"{p1}/{p2}"] = pivot[p2] / pivot[p1]
            result[f"{p2}/{p1}"] = pivot[p1] / pivot[p2]

        result["min/time"] = pivot.min(axis=1)
        result["min/by"] = pivot.idxmin(axis=1)

        result.columns = result.columns.str.replace("egst", "e", regex=False)
        result.columns = result.columns.str.replace("_direct", "d", regex=False)
        result.columns = result.columns.str.replace("_highS3", "h", regex=False)

        # keep only speedup columns
        dfs.append(result[[c for c in result.columns if "/" in c]].reset_index().sort_values(['graph']))

    print("\nEGST-H over EGST")

    print("Time speedup:", end=" ")
    print(", ".join(f"{v:.2f} ({g})" for v, g in zip(dfs[0]["eh/e"], dfs[0]["graph"])))
    print("Max update speedup:", end=" ")
    print(", ".join(f"{v:.2f} ({g})" for v, g in zip(dfs[1]["eh/e"], dfs[1]["graph"])))


    print("\nEGST-D over EGST")
    print("Time speedup:", end=" ")
    print(", ".join(f"{v:.4f} ({g})" for v, g in zip(dfs[0]["ed/e"], dfs[0]["graph"])))
    print("Max update speedup:", end=" ")
    print(", ".join(f"{v:.4f} ({g})" for v, g in zip(dfs[1]["ed/e"], dfs[1]["graph"])))

    print("\nEGST-HD over EGST")
    print("Time speedup:", end=" ")
    print(", ".join(f"{v:.2f} ({g})" for v, g in zip(dfs[0]["edh/e"], dfs[0]["graph"])))
    print("Max update speedup:", end=" ")
    print(", ".join(f"{v:.2f} ({g})" for v, g in zip(dfs[1]["edh/e"], dfs[1]["graph"])))

def print_speedups(df,metric):   
    print(f"\n\t\t\t\t ========= {metric} =========")
   
    # self-merge within epsilon and graph_group
    # pivot so each partition becomes a column
    pivot = df.pivot(
        index=["graph"],
        columns="algo",
        values=metric
    )

    # create speedup columns
    result = pivot.copy()

    for p1, p2 in itertools.combinations(pivot.columns, 2):
        result[f"{p1}/{p2}"] = pivot[p2] / pivot[p1]
        result[f"{p2}/{p1}"] = pivot[p1] / pivot[p2]

    result["min/time"] = pivot.min(axis=1)
    result["min/by"] = pivot.idxmin(axis=1)

    result.columns = result.columns.str.replace("egst", "e", regex=False)
    result.columns = result.columns.str.replace("_direct", "d", regex=False)
    result.columns = result.columns.str.replace("_highS3", "h", regex=False)

    # keep only speedup columns
    speedup_df = result[[c for c in result.columns if "/" in c]].reset_index().sort_values(['graph'])

    print(speedup_df.to_string())


def plot_runtime_per_graph(df, pattern, normalize_to=None, outputdir="plots", grouped=False):
    """
    One figure per pattern:
        - Time bar per algo×partition
        - All other stats overlaying the same bar (starting at y=0)
        - Higher stat -> lighter color
    """

    if grouped:
        df = apply_graph_groups(df, True)
        df = (
            df.groupby(["partition", "algo", "pattern", "graph_group"], as_index=False)
            .agg({m: geo_mean for m in df.select_dtypes(include=["number", "bool"]).columns})
        ).reset_index()
        df["graph"] = df["graph_group"]

    # ---------------------------------------------------------
    # FILTER TO PATTERN
    # ---------------------------------------------------------
    df = df[df["pattern"] == pattern].copy()
    if df.empty:
        print(f"⚠️ No data for pattern '{pattern}'")
        return

    # which columns each stat comes from
    stat_cols = {
        "total": "time",
        "max":   "max_update",
        "99q":   "99q_update",
        "mean":  "mean_update",
        "1q":    "1q_update",
        "min":   "min_update",
    }

    graphs = sorted(df["graph"].unique())
    algos  = sorted(df["algo"].unique())
    parts  = sorted(df["partition"].unique())

    os.makedirs(outputdir, exist_ok=True)

    # ---------------------------------------------------------
    # OPTIONAL NORMALIZATION
    # ---------------------------------------------------------
    df_stat = df.copy()
    if normalize_to is not None:
        ref_algo, ref_part = normalize_to
        normalized_graphs = []
        for g in graphs:
            gdf = df_stat[df_stat["graph"] == g]
            ref_row = gdf[(gdf["algo"] == ref_algo) & (gdf["partition"] == ref_part)]
            if ref_row.empty:
                print(f"⚠️ No reference for graph {g} — skipping")
                continue

            for colname in stat_cols.values():
                ref_val = ref_row.iloc[0][colname]
                if ref_val <= 0:
                    continue
                df_stat.loc[df_stat["graph"] == g, colname] /= ref_val

            normalized_graphs.append(g)

        graphs = [g for g in graphs if g in normalized_graphs]
        if not graphs:
            print("No graphs left after normalization!")
            return

    # ---------------------------------------------------------
    # START FIGURE
    # ---------------------------------------------------------
    fig, ax = plt.subplots(figsize=(12.5, 4.5))

    base_x = np.arange(len(graphs))
    n_bars_per_graph = len(algos) * len(parts)
    bar_width = 0.8 / n_bars_per_graph

    # Precompute per-algo×partition bar offsets
    x_offset = {}
    index = 0
    for algo in algos:
        for part in parts:
            x_offset[(algo, part)] = (index * bar_width) - 0.4
            index += 1

    # print_speedups(df_stat, 'time')
    # print_speedups(df_stat, 'max_update')
    # print_speedups(df_stat, 'mean_update')

    numbers_for_paper(df_stat)

    # MAIN LOOP — time bars + overlays per bar
    for algo in algos:
        for part in parts:
            for i, g in enumerate(graphs):
                row = df_stat[
                    (df_stat["graph"] == g) &
                    (df_stat["algo"] == algo) &
                    (df_stat["partition"] == part)
                ]
                mark_plus = []
                if row.empty:
                    time_val = 0
                    stats = {k: 0 for k in stat_cols}
                    mark_plus.append("")
                else:
                    row = row.iloc[0]
                    time_val = row["time"]
                    stats = {k: row[col] for k, col in stat_cols.items()}

                x_bar = i + x_offset[(algo, part)]
                
                max_steps = df_stat[df_stat["graph"] == g]["steps"].max()
                mark_plus.append("+" if row["steps"] != max_steps else "")


                # Darken by a factor (<1)
                darker_color = np.array(mcolors.to_rgb( palette[algo])) * 0.85

                # ----- Time bar -----
                bars = ax.bar(
                    x_bar,
                    time_val,
                    width=bar_width,
                    color=palette[algo],
                    edgecolor=darker_color,
                    hatch=algo_hatches[algo],
                    linewidth=0.8,
                    label=
                    f"{algo_display[algo]}" if (algo == algos[0] and part == parts[0] and i==0) else None
                )

                for bar, plus in zip(bars, mark_plus):
                    h = bar.get_height()
                    if h > 0:
                        ax.text(
                            bar.get_x() + bar.get_width()/2,
                            h,
                            f"{plus}",
                            # f"{h:.2g}{plus}",
                            ha="center", va="bottom",
                            fontsize=9
                            )
                ax.bar(
                    x_bar,
                    time_val,
                    width=bar_width,
                    facecolor='none',             # transparent fill
                    edgecolor='black', # outer border color
                    linewidth=1,                # thickness of outer border
                )

                # ----- Mean -----
                # ax.hlines(
                #     stats["mean"],
                #     x_bar - bar_width/2,
                #     x_bar + bar_width/2,
                #     colors="black",
                #     linewidth=3,
                #     linestyles=(0,(0.5, 0.5))
                # )

                # # ----- 1q -----
                # ax.hlines(stats["1q"], x_bar - bar_width/2, x_bar + bar_width/2, linestyles=(0,(0.5, 0.5)), colors="black", linewidth=3)

                # ----- 99q -----
                # ax.hlines(stats["99q"], x_bar - bar_width/2, x_bar + bar_width/2, linestyles=(0,(0.5, 0.5)), colors="black", linewidth=3)

                # ----- min -----
                # ax.hlines(stats["min"], x_bar - bar_width/2, x_bar + bar_width/2, linestyles='-', colors="black", linewidth=3)

                # ----- max -----
                ax.hlines(stats["max"], x_bar - bar_width/2, x_bar + bar_width/2, linestyles='-', colors="black", linewidth=3)


    # ---------------------------------------------------------
    # FORMAT AXIS
    # ---------------------------------------------------------
    ax.set_xticks(base_x)
    ax.set_xticklabels([g.replace("_", "\n") for g in graphs], rotation=0, ha="center")
    # ax.set_xticklabels(graphs, rotation=10, ha="right")
    ax.margins(x=0.015)

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax.set_ylabel("Time (s)" if normalize_to is None else "relative time")

    # ax.set_title(
    #     f"All stats overlaid — pattern '{pattern}'"
    #     + (
    #         f" (normalized to {normalize_to[0]}/{normalize_to[1]})"
    #         if normalize_to else ""
    #     )
    # )

    # ---------------------------------------------------------
    # LEGEND
    # ---------------------------------------------------------
    # ------------- PROXY ARTISTS FOR LEGEND -------------

    # Partitions: color + hatch
    partition_handles = []
    for algo in algos:
        for part in parts:
            darker_color = np.array(mcolors.to_rgb( palette[algo])) * 0.85
            hatch=algo_hatches[algo],
            patch = mpatches.Patch(
                facecolor= palette[algo],
                edgecolor=darker_color,
                hatch=hatch,
                label=f"{algo_display[algo]}"
                # /{partition_display[part]}"
                
            )
            partition_handles.append(patch)

    # Stats: linestyle → stat
    stat_handles = []

    # Mean → thick solid
    # stat_handles.append(mlines.Line2D([], [], color='black', linewidth=3, label='Mean Update Time', linestyle=(0,(0.5,0.5))))

    # 1q, 99q → medium solid
    # stat_handles.append(mlines.Line2D([], [], color='black', linewidth=3, label='1q', linestyle=(0,(3,1))))
    # stat_handles.append(mlines.Line2D([], [], color='black', linewidth=3, label='99q', linestyle=(0,(3,1))))

    # # Min, Max → dotted
    # stat_handles.append(mlines.Line2D([], [], color='black', linewidth=3, label='min', linestyle=(0,(1,1))))
    stat_handles.append(mlines.Line2D([], [], color='black', linewidth=3, label='Max Update Time', linestyle='-'))

    # ------------- ADD LEGEND TO AXIS -------------
    ax.legend(
        handles=partition_handles + stat_handles, loc='lower center', ncol=3, bbox_to_anchor=(0.5, 1.02), frameon=True
    )

    plt.tight_layout()

    outname = (
        f"{outputdir}/{pattern}_all_stats_high"
        + (
            f"_norm_{normalize_to[0]}_{normalize_to[1]}"
            if normalize_to else ""
        )
        + ".pdf"
    )

    plt.savefig(outname, dpi=1000)
    plt.close()

    print(f"✓ Saved {outname}")


def plot_pattern_breakdown(df, pattern, outputdir="plots"):
    """
    One figure per pattern.
    Uses geo-mean aggregated data over (algo, partition, pattern).
    """

    # ---------------------------------------------------------
    # FILTER + AGGREGATE
    # ---------------------------------------------------------
    dfp = df[df["pattern"] == pattern].copy()
    if dfp.empty:
        print(f"⚠️ No data for pattern '{pattern}'")
        return

    # Metrics
    time_cols = [
        "structure_addTime", "structure_delTime",
        "structure_toHighTime", "structure_toLowTime",
        "subgraph_addTime", "subgraph_delTime",
        "time"
        
    ]

    op_cols = [
        "op_s0", "op_s1", "op_s2", "op_s3", "op_s4", "op_s5", "op_s6", "op_s7"
    ]

    iter_cols = [
        "iterate_neighbor", "iterate_h"
        ]
    # size_cols = ["H_size_mean"]

    mem_cols = ["memory"]
    cache_cols = ["insn_per_cycle","branch_miss_percent","l1_dcache_miss_percent","cache_miss_percent","dtlb_miss_percent"]

    all_cols = time_cols + op_cols + iter_cols  + mem_cols+cache_cols
    dfp[all_cols] = dfp[all_cols].apply(pd.to_numeric, errors="coerce")


    # Geo-mean aggregation
    gdf = (
        dfp
        .groupby(["algo", "partition"], as_index=False)[all_cols]
        .apply(lambda x: np.exp(np.log(x.replace(0, np.nan)).mean()))
        .reset_index(drop=True)
    )

    # gdf = normalize_relative_to_partition(
    #     gdf,
    #     value_cols=all_cols,
    #     baseline_partition="epstab"
    # )

    algos = sorted(gdf["algo"].unique())
    parts = sorted(gdf["partition"].unique())

    x_labels = [f"{a}\n{p}" for a in algos for p in parts]
    x = np.arange(len(x_labels))

    os.makedirs(outputdir, exist_ok=True)

    # ---------------------------------------------------------
    # FIGURE LAYOUT
    # ---------------------------------------------------------
    fig, axes = plt.subplots(
        nrows=6,
        ncols=1,
        figsize=(22, 18),
        sharex=True,
        gridspec_kw={"height_ratios": [2.5, 2, 2, 1.2, 1.5, 1.2]}
    )

    # =========================================================
    # 1) TIME BREAKDOWN (GROUPED BARS)
    # =========================================================
    ax = axes[0]

    width = 0.8 / len(time_cols)

    print(gdf[['algo', 'partition', 'time']])

    for i, col in enumerate(time_cols):
        values = []
        for a in algos:
            for p in parts:
                row = gdf[(gdf["algo"] == a) & (gdf["partition"] == p)]
                values.append(row[col].iloc[0] if not row.empty else 0)

        ax.bar(
            x + i * width - 0.35,
            values,
            width=width,
            label=col.replace("Time", "")
        )

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax.set_ylabel("time (× baseline)")
    ax.set_title(f"Time breakdown — pattern '{pattern}'")
    ax.legend(ncol=1, fontsize=24, loc='center left',bbox_to_anchor=(1.02, 0.5),)


    # =========================================================
    # 2) OPERATION COUNTS
    # =========================================================
    ax = axes[1]
    width = 0.8 / len(op_cols)

    for i, col in enumerate(op_cols):
        values = []
        for a in algos:
            for p in parts:
                row = gdf[(gdf["algo"] == a) & (gdf["partition"] == p)]
                values.append(row[col].iloc[0] if not row.empty else 0)

        ax.bar(x + i * width - 0.35, values, width=width, label=col)

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax.set_ylabel("count")
    ax.set_title("Operation counts")
    ax.legend(ncol=2, fontsize=24, loc='center left',bbox_to_anchor=(1.02, 0.5))

    # =========================================================
    # 3) CACHE
    # =========================================================
    ax = axes[2]
    width = 0.8 / len(cache_cols)


    for i, col in enumerate(cache_cols):
        values = []
        for a in algos:
            for p in parts:
                row = gdf[(gdf["algo"] == a) & (gdf["partition"] == p)]
                values.append(row[col].iloc[0] if not row.empty else 0)

        ax.bar(x + (i - 1.5) * width, values, width=width, label=col)

    # ax.set_yscale("log")
    ax.set_ylabel("percent")
    ax.set_title("Cache stats")
    ax.legend(ncol=1, fontsize=24, loc='center left',bbox_to_anchor=(1.02, 0.5))


     # =========================================================
    # 4) MEMORY
    # =========================================================
    ax = axes[3]
    width = 0.8 / len(mem_cols)


    for i, col in enumerate(mem_cols):
        values = []
        for a in algos:
            for p in parts:
                row = gdf[(gdf["algo"] == a) & (gdf["partition"] == p)]
                values.append(row[col].iloc[0] if not row.empty else 0)

        ax.bar(x, values, width=width, label=col)

    # ax.set_yscale("log")
    ax.set_ylabel("Memory")
    ax.set_title("Memory stats")
    # ax.legend(ncol=1, fontsize=24, loc='center left',bbox_to_anchor=(1.02, 0.5))


    # =========================================================
    # 5) ITERATIONS
    # =========================================================
    ax = axes[4]
    width = 0.35

    for i, col in enumerate(iter_cols):
        values = []
        for a in algos:
            for p in parts:
                row = gdf[(gdf["algo"] == a) & (gdf["partition"] == p)]
                values.append(row[col].iloc[0] if not row.empty else 0)

        ax.bar(x + (i - 0.5) * width, values, width=width, label=col)

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax.set_ylabel("iterations")
    ax.set_title("Iteration counts")
    ax.legend(ncol=1, fontsize=24, loc='center left',bbox_to_anchor=(1.02, 0.5))


    # ---------------------------------------------------------
    # X AXIS
    # ---------------------------------------------------------
    axes[-1].set_xticks(x)
    axes[-1].set_xticklabels(x_labels, rotation=45, ha="right")

    plt.tight_layout()

    outname = f"{outputdir}/{pattern}_breakdown.pdf"
    plt.savefig(outname, dpi=1000)
    plt.close()

    print(f"✓ Saved {outname}")

def main():
    # Load the data from the CSV file
    input_file = "results.csv"
    Path("plots").mkdir(parents=True, exist_ok=True)
    df = pd.read_csv(input_file)

    # If recompute_mode == 1 → partition becomes "partition_soft"
    df["algo"] = df["algo"] + df["directVertexChange"].map(lambda v: "_direct" if v == 1 else "")
    df["algo"] = df["algo"] + df["s3HighAnchors"].map(lambda v: "_highS3" if v == 1 else "")
    print(df["algo"].unique())

    # Then drop the old column
    df = df.drop(columns=["directVertexChange"])
    df = df.drop(columns=["s3HighAnchors"])

    
    # ---------------------------------------------------------
    # FILTER GRAPHS WHERE ALL ALGORITHMS ARE PRESENT
    # ---------------------------------------------------------
    # Get the list of unique algorithms
    all_algos = df["algo"].unique()

    # Group by graph and filter graphs where all algorithms are present
    graphs_with_all_algos = df.groupby("graph").filter(
        lambda g: set(g["algo"].unique()) == set(all_algos)
    )["graph"].unique()

    # Filter the DataFrame to include only these graphs
    df = df[df["graph"].isin(graphs_with_all_algos)]

    # ------------------------------------------
    # Median over runs
    # ------------------------------------------
    group_cols = ["graph", "algo", "partition", "pattern"]
    df = df.groupby(group_cols).median().reset_index()
    
    plot_runtime_per_graph(df, 'all', grouped=True)

if __name__ == "__main__":
    main()
