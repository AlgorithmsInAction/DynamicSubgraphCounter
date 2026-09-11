import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys, os
import matplotlib.colors as mcolors
import matplotlib.patches as mpatches
import matplotlib.lines as mlines
import itertools
from collections import defaultdict
import matplotlib.patheffects as pe


sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import palette, pattern_hatches, partition_display, algo_display, apply_graph_groups, geo_mean, graph_info,get_label
from util import ComputerModernLogFormatter


def print_paper_summary(df):
    """Print a readable summary matching the numbers cited in the paper."""
    df = df.copy()
    df['algo_part'] = df['algo'] + "_" + df['partition']

    comparisons = {
        # (numerator, denominator, label)
        'Partition tuning (hhh)':  ('hhh_epstab_0.417',  'hhh_epstab_late_0.2',    'h_e0.417 / h_le0.2'),
        'Partition tuning (degst)': ('egst_high_direct_hindex2.0', 'egst_high_direct_hindex1.0625', 'e_h2.0 / e_h1.0625'),
        'Best vs best':           ('egst_high_direct_hindex1.0625', 'hhh_epstab_late_0.2', 'e_h1.0625 / h_le0.2'),
    }

    wiki_def = ['de_wiki', 'it_wiki', 'fr_wiki']  # adjust names to match your data

    for metric in ['time', 'memory']:
        print(f"\n{'='*60}")
        print(f"  PAPER SUMMARY — {metric.upper()}")
        print(f"{'='*60}")

        pivot = df.pivot(index="graph", columns="algo_part", values=metric)

        for label, (num, den, tag) in comparisons.items():
            if num not in pivot.columns or den not in pivot.columns:
                print(f"\n  [{label}] — columns not found, skipping")
                continue

            ratio = pivot[num] / pivot[den]
            valid = ratio.dropna()
            valid_no_wiki = valid[~valid.index.isin(wiki_def)]

            geo_all = np.exp(np.log(valid[valid > 0]).mean()) if len(valid[valid > 0]) else np.nan
            geo_no_wiki = np.exp(np.log(valid_no_wiki[valid_no_wiki > 0]).mean()) if len(valid_no_wiki[valid_no_wiki > 0]) else np.nan

            print(f"\n  [{label}]  {tag}")
            print(f"    Geo mean (all):          {geo_all:.2f}x")
            print(f"    Geo mean (no wiki D/E/F):{geo_no_wiki:.2f}x")
            print(f"    Range:                   {valid.min():.2f}x – {valid.max():.2f}x")

            if metric == 'memory' and 'best' in label.lower():
                mem_ratio = pivot[num] / pivot[den]  # inverted: how much more mem does den use
                inv = pivot[den] / pivot[num]
                geo_inv = np.exp(np.log(inv.dropna()[inv.dropna() > 0]).mean())
                print(f"    hhh uses {(geo_inv - 1)*100:.1f}% more memory (geo mean)")

            # per-graph detail
            # print(f"    {'Graph':<25} {'Speedup':>8}")
            # print(f"    {'-'*34}")
            # for g in sorted(valid.index):
            #     flag = " **TIMEOUT**" if g in wiki_def and 'best' in label.lower() else ""
            #     print(f"    {g:<25} {valid[g]:>8.2f}x{flag}")

    # Wiki timeout details
    print(f"\n{'='*60}")
    print("  WIKI TIMEOUT DETAILS")
    print(f"{'='*60}")
    pivot_t = df.pivot(index="graph", columns="algo_part", values='time')
    pivot_s = df.pivot(index="graph", columns="algo_part", values='steps') if 'steps' in df.columns else None
    pivot_total = df.pivot(index="graph", columns="algo_part", values='total_steps') if 'total_steps' in df.columns else None

    for g in wiki_def:
        if g in pivot_t.index:
            print(f"\n  {g}:")
            for col in pivot_t.columns:
                t = pivot_t.loc[g, col]
                print(f"    {col:<45} {t/3600:>8.1f} h")


def print_speedups(df,metric):
    print(f"\n\t\t\t\t ========= {metric} =========")
    # self-merge within epsilon and graph_group
    # pivot so each partition becomes a column
    df['algo_part'] =  df['algo'] +  "_" +df['partition']
    pivot = df.pivot(
        index=["graph"],
        columns="algo_part",
        values=metric
    )

    # create speedup columns
    result = pivot.copy()

    for p1, p2 in itertools.combinations(pivot.columns, 2):
        result[f"{p1}/{p2}"] = pivot[p1] / pivot[p2]
        result[f"{p2}/{p1}"] = pivot[p2] / pivot[p1]

    result["min/time"] = pivot.min(axis=1)
    result["min/by"] = pivot.idxmin(axis=1)

    result.columns = result.columns.str.replace("egst_high_direct", "e", regex=False)
    result.columns = result.columns.str.replace("hhh", "h", regex=False)
    result.columns = result.columns.str.replace("epstab_", "e", regex=False)
    result.columns = result.columns.str.replace("late_", "l", regex=False)
    result.columns = result.columns.str.replace("hindex", "h", regex=False)

    # keep only speedup columns
    speedup_df = result[[c for c in result.columns if "/" in c]].reset_index().sort_values(['graph'])

    print(speedup_df[['graph','e_h2.0/e_h1.0625', 'h_e0.417/h_el0.2','e_h1.0625/h_el0.2']].to_string())
    # print(speedup_df[['graph','e_e0.4/e_h','e_h/h_e0.2','e_e0.4/h_e0.2','e_e0.33/h_e0.2','h_e0.2/e_e0.33']].to_string())
    # print(speedup_df.to_string())

    def geo_mean_no_nan(series):
        series = series.dropna()          # remove NaN
        series = series[series > 0]       # geometric mean requires positive values
        if len(series) == 0:
            return np.nan
        return np.exp(np.log(series).mean())
    
    # ---- GEO MEAN SINGLE LINE ----
    geo_df = speedup_df[['e_h2.0/e_h1.0625', 'h_e0.417/h_el0.2','e_h1.0625/h_el0.2','h_el0.2/h_e0.417','h_el0.2/e_h1.0625','e_h1.0625/h_e0.417']].apply(geo_mean_no_nan).to_frame().T
    geo_df.index = ["geo_mean"]

    print("\nGeometric mean speedups: (all instances)")
    print(geo_df.to_string())

    # ---- GEO MEAN SINGLE LINE ----
    speedup_df_no_wiki = speedup_df[(~speedup_df['graph'].str.endswith('wiki') | speedup_df['graph'].str.contains('simple') | speedup_df['graph'].str.contains('nl') | speedup_df['graph'].str.contains('pl') )]
    geo_df = speedup_df_no_wiki[['e_h2.0/e_h1.0625', 'h_e0.417/h_el0.2','e_h1.0625/h_el0.2','h_el0.2/h_e0.417','h_el0.2/e_h1.0625','e_h1.0625/h_e0.417']].apply(geo_mean_no_nan).to_frame().T
    geo_df.index = ["geo_mean"]

    print("\nGeometric mean speedups: (no wiki D,E,F)")
    print(geo_df.to_string())


def plot_runtime_per_graph(
    df1,
    variants_dict1,
    df2,
    variants_dict2,
    pattern,
    graph_info,
    normalize_to=None,
    outputdir="plots",
    grouped=False,
):

    plt.rcParams.update({"font.size": 40})
    os.makedirs(outputdir, exist_ok=True)

    # ---------------------------------------------------------
    # FILTER TO PATTERN
    # ---------------------------------------------------------
    df1 = df1[df1["pattern"] == pattern].copy()
    df2 = df2[df2["pattern"] == pattern].copy()

    if df1.empty and df2.empty:
        print(f"⚠️ No data for pattern '{pattern}'")
        return

    # ---------------------------------------------------------
    # NORMALIZATION
    # ---------------------------------------------------------
    def normalize_df(df, normalize_to):
        if normalize_to is None:
            return df

        ref_algo, ref_part = normalize_to
        graphs = df["graph"].unique()

        for g in graphs:
            gdf = df[df["graph"] == g]
            ref_row = gdf[
                (gdf["algo"] == ref_algo)
                & (gdf["partition"] == ref_part)
            ]
            if ref_row.empty:
                continue

            ref_val = ref_row.iloc[0]["time"]
            if ref_val > 0:
                df.loc[df["graph"] == g, "time"] /= ref_val

        return df

    df1 = normalize_df(df1, normalize_to)
    df2 = normalize_df(df2, normalize_to)

    # ---------------------------------------------------------
    # SORT GRAPHS BY GROUP
    # ---------------------------------------------------------
    # graphs = sorted(graph_info.keys(), key=lambda g: graph_info[g][0])
    graphs = list(graph_info.keys())
    graphs1 = set(df1["graph"].unique())
    graphs2 = set(df2["graph"].unique())

    # ---------------------------------------------------------
    # CREATE SPACED X POSITIONS (group gaps)
    # ---------------------------------------------------------
    base_x = []
    current = 0
    gap = 0.5

    last_group = None
    for g in graphs:
        group, _ = graph_info[g]

        if last_group is not None and group != last_group:
            current += gap

        base_x.append(current)
        current += 1
        last_group = group

    base_x = np.array(base_x)

    # ---------------------------------------------------------
    # FIGURE
    # ---------------------------------------------------------
    if grouped:
        fig, ax = plt.subplots(figsize=(12.5, 6))
    else:
        fig, ax = plt.subplots(figsize=(40, 7))

    # ---------------------------------------------------------
    # MAIN LOOP
    # ---------------------------------------------------------
    for i, g in enumerate(graphs):

        if g in graphs1:
            df_current = df1
            variants_current = variants_dict1
        elif g in graphs2:
            df_current = df2
            variants_current = variants_dict2
        else:
            continue

        n_bars = len(variants_current)
        bar_width = 0.85 / n_bars

        offsets = [
            -0.4 + j * bar_width + bar_width / 2
            for j in range(n_bars)
        ]

        for j, (algo, part, color) in enumerate(variants_current):

            row = df_current[
                (df_current["graph"] == g)
                & (df_current["algo"] == algo)
                & (df_current["partition"] == part)
            ]

            if row.empty:
                continue

            time_val = row.iloc[0]["time"]
            x_bar = base_x[i] + offsets[j]

            darker_color = np.array(
                mcolors.to_rgb(color)
            ) * 0.85
            if not grouped:
                darker_color *= 0.9

            ax.bar(
                x_bar,
                time_val,
                width=bar_width,
                color=color,
                edgecolor=darker_color,
                hatch=pattern_hatches[part],
                linewidth=0.01,
            )
            # ---------------------------------------------------------
            # MARK LARGE VALUES (> 86000) WITH "+"
            # ---------------------------------------------------------
            if time_val > 86000:
                ax.text(
                    x_bar,
                    time_val * 1.05,   # slightly above bar (important for log scale)
                    "+",
                    ha="center",
                    va="bottom",
                    fontsize=10,
                    fontweight="bold",
                    path_effects=[
                        pe.Stroke(linewidth=2, foreground="black"),
                        pe.Normal(),
                    ],
                )

    # ---------------------------------------------------------
    # X-AXIS: IDs (bottom row)
    # ---------------------------------------------------------
    ids = [graph_info[g][1] for g in graphs]

    ax.set_xticks(base_x)
    ax.set_xticklabels(
        ids,
        # rotation=0 if grouped else 90,
        ha="center",
    )
    ax.margins(x=0.01)


    # ---------------------------------------------------------
    # GROUP LABELS (second row)
    # ---------------------------------------------------------
    group_positions = defaultdict(list)

    for i, g in enumerate(graphs):
        group, _ = graph_info[g]
        group_positions[group].append(base_x[i])

    for group, positions in group_positions.items():
        center = np.mean(positions)

        ax.text(
            center,
            -0.17,
            group,
            ha="center",
            va="top",
            transform=ax.get_xaxis_transform(),
            fontsize=30,
        )

    # ---------------------------------------------------------
    # Y-AXIS
    # ---------------------------------------------------------
    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax.set_ylabel(
        "Time (s)" if normalize_to is None else "Relative Time"
    )

    # ---------------------------------------------------------
    # LEGEND
    # ---------------------------------------------------------
    legend_variants = list(variants_dict1) + list(variants_dict2)
    seen = set()
    handles = []

    for (algo, part, color) in legend_variants:
        if (algo, part) in seen:
            continue
        seen.add((algo, part))

        darker_color = np.array(
            mcolors.to_rgb(color)
        ) * 0.85
        if not grouped:
            darker_color *= 0.9
        print(algo)
        patch = mpatches.Patch(
            facecolor=color,
            edgecolor=darker_color,
            hatch=pattern_hatches[part],
            label=get_label(algo,part),
            # f"{algo_display[algo]}/{partition_display[part]}",
        )
        handles.append(patch)

    ax.legend(
        handles=handles,
        loc="lower center",
        ncol=6,
        bbox_to_anchor=(0.5, 1.02),
        frameon=True,
    )

    # ---------------------------------------------------------
    # LAYOUT FIX (important for group labels)
    # ---------------------------------------------------------
    plt.subplots_adjust(bottom=0.25)
    plt.tight_layout()

    # ---------------------------------------------------------
    # SAVE
    # ---------------------------------------------------------
    outname = (
        f"{outputdir}/{pattern}_comp"
        + (
            f"_norm_{normalize_to[0]}_{normalize_to[1]}"
            if normalize_to
            else ""
        )
        + ".pdf"
    )

    plt.savefig(outname, dpi=1000)
    plt.close()

    print(f"✓ Saved {outname}")

def main():

    input_file ="results_full_all.csv"
    # input_file ="res_pre.csv"
    print(f"Processing {input_file} ...")
    df = pd.read_csv(input_file)

    df["partition"] = df["partition"] + df["recompute_mode"].map(
    lambda v: "_soft" if v == 1 else ("_lazy" if v == 2 else ("_late" if v == 3 else ""))
    )

    # Check if the value of the "partition" column is "epstab" for each row
    df["partition"] = df.apply(
        lambda row: row["partition"] + "_" + str(row["epsilon"])
        if "epstab" in row["partition"]
        else row["partition"] + (str(row["epsilon"]) if row["epsilon"] > 0 else ""),
        axis=1
    )
    print(df["partition"].unique())


    # df["partition_merged"] = "hindex"
    # df.loc[(df["partition"] == "epstab") & (df["algo"] == "hhh"), "partition_merged"] = "epstab_lazy_0.2"
    # df.loc[(df["partition"] == "epstab") & (df["algo"] == "egst") & (df["graph"].astype(str).str.endswith("wiki")), "partition_merged"] = "epstab_lazy_0.4"
    # df.loc[(df["partition"] == "epstab") & (df["algo"] == "egst") & (~df["graph"].astype(str).str.endswith("wiki")), "partition_merged"] = "epstab_lazy_0.33"
    # df["partition"] = df["partition_merged"] 

    mask = df["algo"] == "egst"
    df.loc[mask, "algo"] = (
        df.loc[mask, "algo"]
        + df.loc[mask, "s3HighAnchors"].map(lambda v: "_high" if v == 1 else "")
        + df.loc[mask, "directVertexChange"].map(lambda v: "_direct" if v == 1 else "")
    )
    # df["algo"] = df["algo"] + df["s3HighAnchors"].map(lambda v: "_high" if v == 1 else "")
    # df["algo"] = df["algo"] + df["directVertexChange"].map(lambda v: "_direct" if v == 1 else "")
    print(df["algo"].unique())

    df = df.drop(columns=["directVertexChange"])
    df = df.drop(columns=["s3HighAnchors"])


    group_cols = ["graph", "algo", "partition", "steps", "pattern"]
    df = df.groupby(group_cols).median().reset_index()


    idx = df.groupby(
        ["graph", "algo", "partition", "pattern"]
    )["steps"].transform("max") == df["steps"]

    df = df[idx].reset_index(drop=True)

        
    df["graph"] = df["graph"].str.replace(r"ia-enron-email-dynamic","enron",regex=True)
    df["graph"] = df["graph"].str.replace(r"-temporal","",regex=True)
    df["graph"] = df["graph"].str.replace(r"Hyperlinks","",regex=True)
    df["graph"] = df["graph"].str.replace(r"^(sx-|soc-|ia-)|(_2m)$","",regex=True)
    df["graph"] = df["graph"].str.replace(r"(_f)$","",regex=True)

    print_speedups(df, 'time')
    print_speedups(df, 'memory')
    print_speedups(df, 'steps')
    print_paper_summary(df)

    # print(df[['graph','algo', 'partition', 'steps', 'time']].to_string())

    df_wiki=df[df["graph"].astype(str).str.endswith('wiki')]
    variants_to_plot= [
        # ("egst_high_direct", "epstab_lazy_0.4"),
        ("egst_high_direct", "hindex2.0", "#56B4E9"),
        ("egst_high_direct", "hindex1.0625", "#009E73"),
        ("hhh", "epstab_0.417", "#E69F00"),
        ("hhh", "epstab_late_0.2", "#CC79A7"),
        # ("hhh", "epstab_lazy_0.2", "#D55E00"),
    ]
    rest = df[~df["graph"].astype(str).str.endswith("wiki")]

    print(df_wiki[['graph','time','algo_part']])
    # print(rest)

    plot_runtime_per_graph(df_wiki, variants_to_plot,rest,variants_to_plot, 'all',graph_info, grouped=False)

if __name__ == "__main__":
    main()
