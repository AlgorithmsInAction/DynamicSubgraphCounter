import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import matplotlib.patches as mpatches
import sys, os
import re
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
import itertools

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import palette, pattern_hatches, algo_display, partition_display, graph_info,get_label,geo_mean
from util import ComputerModernLogFormatter

# ==========================
# CONFIG
# ==========================
RESULTS_CSV ="results_150k.csv"
DATA_DIR = Path("resultsFR")
OUT_DIR = Path("plots")
OUT_DIR.mkdir(exist_ok=True)


# Variants
VARIANTS = {
    "oba_none": ("oba", "none"),
    "escape_none": ("escape", "none"),
    "v_hhh_epstab_0417": ("v_hhh", "epstab_0417"),
    "egst_hindex2.0": ("egst", "hindex2.0"),
    "hhh_epstab_late_02": ("hhh", "epstab_late_02"),
    "egst_high_direct_epstab_hindex1.0625": ("egst_high_direct", "hindex1.0625"),


}

metric_display ={
    "time" : "Total\n Time",
    "max_update" : "Max\n Update",
    "99q_update" : "99-q\n Update",
}

METRICS = {
    "time",
    "max_update",
    "99q_update"
}



OP_COL ="op_s3"
def find_x_vals(graph, algo, partition):
    p=partition

    if "hindex2.0" in partition:
        p="hindex_2"
    if "hindex1.0625" in partition:
        p="hindex_1716"

    pattern = re.compile(rf"{re.escape(graph)}_150k_all_{algo}_{p}_time(\d+)\.txt")
    xs = [int(m.group(1)) for p in DATA_DIR.iterdir() if (m := pattern.fullmatch(p.name))]
    return sorted(xs)


def load_median_over_x(graph, algo, partition):
    xs = find_x_vals(graph, algo, partition)

    if not xs:
        raise FileNotFoundError(f"No files for {graph} {algo} {partition}")

    # Columns to ensure exist
    additional_cols = [
        "recomputation",
        OP_COL,
        "s3_buckets",
        "s0_buckets",
        "s1_buckets",
        "s2_buckets",
        "s4_buckets",
        "s5_buckets",
        "s6_buckets",
        "s7_buckets",
        "vLV_buckets","uLv_buckets","t_buckets","uLLv_buckets","cLV_buckets","pLL_buckets","uHv_buckets","cL_buckets",
        "s3_MapInsert_Time",
        "H_size"
    ]

    dfs = []
    for x in xs:
        p=partition
        if "hindex2.0" in partition:
            p="hindex_2"
        if "hindex1.0625" in partition:
            p="hindex_1716"
        path = DATA_DIR / f"{graph}_150k_all_{algo}_{p}_time{x}.txt"

        df = pd.read_csv(path)

        # Ensure all required columns exist
        for col in additional_cols:
            if col not in df.columns:
                df[col] = 0

        # If OP_COL exists, compute diff
        df[OP_COL] = df[OP_COL].diff().fillna(0)
        if "s0_buckets" in df:
            df["s0_buckets"]  = df["s0_buckets"] .diff().fillna(0)
            df["s1_buckets"]  = df["s1_buckets"] .diff().fillna(0)
            df["s2_buckets"]  = df["s2_buckets"] .diff().fillna(0)
            df["s3_buckets"]  = df["s3_buckets"] .diff().fillna(0)
            df["s4_buckets"]  = df["s4_buckets"] .diff().fillna(0)
            df["s5_buckets"]  = df["s5_buckets"] .diff().fillna(0)
            df["s6_buckets"]  = df["s6_buckets"] .diff().fillna(0)
            df["s7_buckets"]  = df["s7_buckets"] .diff().fillna(0)

        if "op_s1" in df:
            df["op_s0"] = df["op_s0"].diff().fillna(0)
            df["op_s1"] = df["op_s1"].diff().fillna(0)
            df["op_s2"] = df["op_s2"].diff().fillna(0)
            df["op_s4"] = df["op_s4"].diff().fillna(0)
            df["op_s5"] = df["op_s5"].diff().fillna(0)
            df["op_s6"] = df["op_s6"].diff().fillna(0)
            df["op_s7"] = df["op_s7"].diff().fillna(0)

            df["ops"] = df["op_s3"]+df["op_s0"]+df["op_s1"]+df["op_s2"]+df["op_s4"]+df["op_s5"]+df["op_s6"]+df["op_s7"]
        else:
            df["ops"]=0

        df["vLV_buckets"]  = df["vLV_buckets"] .diff().fillna(0)
        df["uLv_buckets"]  = df["uLv_buckets"] .diff().fillna(0)
        df["t_buckets"]  = df["t_buckets"] .diff().fillna(0)
        df["uLLv_buckets"]  = df["uLLv_buckets"] .diff().fillna(0)
        df["cLV_buckets"]  = df["cLV_buckets"] .diff().fillna(0)
        df["pLL_buckets"]  = df["pLL_buckets"] .diff().fillna(0)
        df["uHv_buckets"]  = df["uHv_buckets"] .diff().fillna(0)
        df["cL_buckets"]  = df["cL_buckets"] .diff().fillna(0)


        df["s3_MapInsert_Time"]  = df["s3_MapInsert_Time"] .diff().fillna(0)

        df["x"] = x
        dfs.append(df)

    all_df = pd.concat(dfs, ignore_index=True)

    # Aggregate by step
    med = all_df.groupby("step", as_index=False).agg(
        time=("time", "median"),
        recomputation=("recomputation", "max"),
        op=(OP_COL, "max"),
        s3_buckets=("s3_buckets", "max"),
        s0_buckets=("s0_buckets", "max"),
        s1_buckets=("s1_buckets", "max"),
        s2_buckets=("s2_buckets", "max"),
        s4_buckets=("s4_buckets", "max"),
        s5_buckets=("s5_buckets", "max"),
        s6_buckets=("s6_buckets", "max"),
        s7_buckets=("s7_buckets", "max"),
        vLV_buckets=("vLV_buckets", "max"),
        uLv_buckets=("uLv_buckets", "max"),
        t_buckets=("t_buckets", "max"),
        uLLv_buckets=("uLLv_buckets", "max"),
        cLV_buckets=("cLV_buckets", "max"),
        pLL_buckets=("pLL_buckets", "max"),
        uHv_buckets=("uHv_buckets", "max"),
        cL_buckets=("cL_buckets", "max"),
        s3_MapInsert_Time=("s3_MapInsert_Time", "max"), 
        H_size=("H_size", "max"),
        ops=("ops", "max"),
    )
    return med, len(xs)


def plot_big_overview():
    df = pd.read_csv(RESULTS_CSV)
    group_cols = ["graph", "algo", "partition", "pattern"]

    num_cols = df.select_dtypes(include=np.number).columns
    num_cols = [c for c in num_cols if c not in group_cols]

    df_all = df.groupby(group_cols)[num_cols].median().reset_index()
    print_speedups(df_all)

    fig, ax = plt.subplots(1, 1, figsize=(17, 8))
    ax2 = ax.twinx()

    metrics_ordered = ["time", "max_update", "99q_update"]
    spacing_between_metrics = len(VARIANTS) + 1
    x_base = 0
    metric_centers = []
    left_positions, left_data, left_colors, left_hatches = [], [], [], []
    right_positions, right_data, right_colors, right_hatches = [], [], [], []

    # Extra color so that hhh is not the only color used twice
    special_combo = ("hhh", "epstab_0417")
    special_color = "#F0E442"

    for metric in metrics_ordered:
        for i, (variant, (algo, part)) in enumerate(VARIANTS.items()):
            values = df_all[(df_all["algo"] == algo) & (df_all["partition"] == part)][metric]
            pos = x_base + i

            color = special_color if (algo, part) == special_combo else palette[algo]

            if metric == "time":
                left_positions.append(pos)
                left_data.append(values)
                left_colors.append(color)
                left_hatches.append(pattern_hatches[part])
            else:
                right_positions.append(pos)
                right_data.append(values)
                right_colors.append(color)
                right_hatches.append(pattern_hatches[part])

        if left_data:
            bp_left = ax.boxplot(left_data, positions=left_positions, widths=1,
                                 patch_artist=True, showfliers=True)
            for patch, color, hatch in zip(bp_left["boxes"], left_colors, left_hatches):
                patch.set_facecolor(color)
                patch.set_hatch(hatch)
                patch.set_edgecolor("black")

        if right_data:
            bp_right = ax2.boxplot(right_data, positions=right_positions, widths=1,
                                   patch_artist=True, showfliers=True)
            for patch, color, hatch in zip(bp_right["boxes"], right_colors, right_hatches):
                patch.set_facecolor(color)
                patch.set_hatch(hatch)
                patch.set_edgecolor("black")

        center = x_base + (len(VARIANTS) - 1) / 2
        metric_centers.append(center)
        x_base += spacing_between_metrics

        if metric == "time":
            ax.axvline(x=x_base - 1, linestyle=":", color="black", linewidth=1)

    x_base -= spacing_between_metrics

    # Axis settings
    ax.set_xticks(metric_centers)
    ax.set_xticklabels([metric_display[m] for m in metrics_ordered], rotation=30)
    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax2.set_yscale("log")
    ax2.yaxis.set_major_formatter(ComputerModernLogFormatter())
    ax2.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
    ax.set_ylabel("Total Time")
    ax2.set_ylabel("Update Time")

    # Horizontal grid
    for y in ax.get_yticks():
        ax.hlines(y=y, xmin=min(left_positions)-0.5, xmax=max(left_positions)+1,
                  colors='lightgray', linestyles=':', linewidth=0.5)
    for y in ax2.get_yticks():
        ax2.hlines(y=y, xmin=min(right_positions)-1, xmax=max(right_positions)+0.5,
                  colors='lightgray', linestyles=':', linewidth=0.5)

    # Legend
    algos = list(dict.fromkeys(algo for variant, (algo, part) in VARIANTS.items()))
    parts = list(dict.fromkeys(part for variant, (algo, part) in VARIANTS.items()))
    algo_handles = [mpatches.Patch(facecolor=palette[a], edgecolor='black', label=algo_display[a]) for a in algos]
    part_handles = [mpatches.Patch(facecolor='white', edgecolor='black', hatch=pattern_hatches[p], label=partition_display[p]) for p in parts]
    algo_label = mpatches.Patch(facecolor='none', edgecolor='none', label="Algorithm:")
    part_label = mpatches.Patch(facecolor='none', edgecolor='none', label="Partition:")

    def pad(handles, target_len):
        return handles + [mpatches.Patch(facecolor='none', edgecolor='none', label="")] * (target_len - len(handles))

    max_len = max(len(algo_handles), len(part_handles))
    handles = [algo_label] + pad(algo_handles, max_len) + [part_label] + pad(part_handles, max_len)
    fig.tight_layout(rect=[0, 0, 0.6, 1])
    unique_combos = list(dict.fromkeys((algo, part) for variant, (algo, part) in VARIANTS.items()))

    # Create one patch per combination
    combo_handles=[]
    for algo, part in unique_combos:
        color = special_color if (algo, part) == special_combo else palette[algo]
        combo_handles.append(
            mpatches.Patch(
                facecolor=color,
                edgecolor='black',
                hatch=pattern_hatches[part],
                label=get_label(algo,part)
            )
        )
    # Single legend
    ax.legend(handles=combo_handles, ncol=1, loc='upper left', bbox_to_anchor=(1.2, 1.05), frameon=False)
        # =====================
    # INSET PLOT for a single graph
    # =====================
    VARIANTS_SMALL = VARIANTS
    # {
        # "oba_none": ("oba", "none"),
        # "escape_none": ("escape", "none"),
        # "egst_hindex": ("egst", "hindex"),
        # "hhh_epstab_0417": ("hhh", "epstab_0417"),
        # "hhh_epstab_late_02": ("hhh", "epstab_late_02"),
        # "egst_high_direct_epstab_lazy_0333": ("egst_high_direct", "epstab_lazy_0333"),
    # }

    # Adjust these numbers to fit nicely under the legend
    small_ax = fig.add_axes([0.67, 0.1, 0.31, 0.4])  

    graph = "fr_wiki"  # Example graph
    legend_handles_small = {}
    top_lim=0.1
    inset_peak = 0.0
    inset_steps = 0

    for variant, (algo, part) in VARIANTS_SMALL.items():
        try:
            df_step, _ = load_median_over_x(graph, algo, part)
        except FileNotFoundError:
            raise SystemExit(f"Missing per-update data for static inset: {graph} {algo} {part}")

        inset_peak = max(inset_peak, float(df_step["time"].max()))
        inset_steps = max(inset_steps, int(df_step["step"].max()))
        
        c = special_color if (algo, part) == special_combo else palette[algo]
        
        label = get_label(algo, part)
        line, = small_ax.plot(df_step["step"], df_step["time"],
                            color=c, label=label, linewidth=1.5)
        legend_handles_small[label] = line

        # # recomputation markers
        # recomp = df_step[df_step.get("recomputation", 0) > 0].copy()
        # if not recomp.empty and ('lazy' not in part):
        #     recomp["time"]= recomp["time"].clip(upper=top_lim)

        #     sc = small_ax.scatter(recomp["step"], recomp["time"], color="red", marker="x", zorder=5)
        #     legend_handles_small.setdefault("Major Rebalancing", sc)

        # # --- S3 Rehash Points ---
        # for bucket in ["s3_buckets"
        #             #    ,"vLV_buckets","uLv_buckets","t_buckets","uLLv_buckets","cLV_buckets","pLL_buckets","uHv_buckets","cL_buckets"
        #                ]:
        #     if bucket in df_step.columns:
        #         threshold = 500_000
        #         s3_rehash = df_step[df_step[bucket] >= threshold].copy()
        #         if not s3_rehash.empty:
        #             s3_rehash["time"]=s3_rehash["time"].clip(upper=top_lim)

        #             sc3 = small_ax.scatter(
        #                 s3_rehash["step"],
        #                 s3_rehash["time"],
        #                 color="black",
        #                 marker="x",
        #                 zorder=5
        #             )
        #             legend_handles_small.setdefault("Hashmap Rehash ($>\\frac{1}{2}$M)", sc3)


        # # Many S3 Ops
        # if "op" in df_step.columns and df_step["ops"].max() > 0:
        #     threshold = 0.9
        #     threshold_ops = 0.5 * df_step["ops"].max()
        #     s3 = df_step[(df_step["ops"] > threshold_ops) & (df_step["op"]/df_step["ops"] >= threshold)]
        #     if not s3.empty and s3['op'].min() > 0:
        #         sc2 = small_ax.scatter(s3["step"], s3["time"], color="orange", marker=".", zorder=5)
        #         legend_handles_small.setdefault("Many S3 Ops", sc2)

    small_ax.set_title(graph_info[graph][0]+"-"+graph_info[graph][1])
    small_ax.set_xlabel("Update Step")
    small_ax.set_ylabel("Time (s)")
    # small_ax.set_yscale("log")
    # Keep the original full-run scale; expose microsecond toy timings rather
    # than flattening them against the fixed 0.1-second upper limit.
    if inset_steps <= 2000:
        top_lim = max(inset_peak * 1.1, 1e-12)
    small_ax.set_ylim(bottom=0,top=top_lim)
    small_ax.tick_params(axis='both', which='major')
    small_ax.grid(True, linestyle=':', linewidth=0.5)


    # (a) for the main boxplot
    ax.text(
        -0.07, 1.02,  # x, y in axes coordinates (0-1)
        "(a)",
        transform=ax.transAxes,
        fontsize=30,
        fontweight="bold",
        va="top",
        ha="right"
    )

    # (b) for the small external plot
    small_ax.text(
        -0.15, 1.05,  # slightly more offset for small plot
        "(b)",
        transform=small_ax.transAxes,
        fontsize=30,
        fontweight="bold",
        va="top",
        ha="right"
    )
    # Save figure
    out_file = OUT_DIR / "static_compare.pdf"
    plt.savefig(out_file)
    plt.close()
    print(f"Saved {out_file}")


def print_speedups(df):
    metrics = ["time","max_update","99q_update"]
    df["algo_part"] = df["algo"] + "|" + df["partition"]


    df_geo = (
        df.groupby(["algo_part", "partition"])
        .agg({m: geo_mean for m in metrics})   # no list → no MultiIndex
        .reset_index()
    )


    df_pivot = df.pivot(index="graph", columns="algo_part", values="time")
    dfs=[]
    for metric in metrics:
        pairs = [
            ('escape|none','oba|none'),
            ('oba|none','egst_high_direct|hindex1.0625'),
            ('oba|none','egst|hindex2.0'),
            ('oba|none','v_hhh|epstab_0417'),
            ('oba|none','hhh|epstab_late_02'),
        ]


       # make lookup
        s = df_geo.set_index("algo_part")[metric]

        results = []
        for a, b in pairs:
            results.append({
                "pair": f"{a}/{b}",
                "ratio": s[a] / s[b]
            })
            # print(a,b)
            # print((df_pivot[a] / df_pivot[b]))


        df_ratio = pd.DataFrame(results)
        print(df_ratio)


       


   
# ==========================
# MAIN
# ==========================
def main():
    plot_big_overview()

if __name__=="__main__":
    main()
