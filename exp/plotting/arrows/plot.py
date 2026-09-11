import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.colors import to_rgb
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import pattern_display,pattern_palette
from util import ComputerModernLogFormatter

INPUT = "results_orig_partition.csv"
# INPUT = "results_lazy_partition.csv"
OUTDIR = "plots"

os.makedirs(OUTDIR, exist_ok=True)

df = pd.read_csv(INPUT)

# print(df[df['epsilon']=='fCliques'])

# -------------------------------------------------
# median over runs
# -------------------------------------------------
group_cols = ["graph", "algo", "partition", "epsilon", "use_all_aux", "aux_for_t", "pattern"]


num_cols = df.select_dtypes(include=np.number).columns
num_cols = [c for c in num_cols if c not in group_cols]

df = (
    df.groupby(group_cols)[num_cols]
    .median()
    .reset_index()
)

# best_eps = df.loc[
#     df.groupby(["graph","pattern", "use_all_aux", "aux_for_t"])["time"].idxmin()
# ]

# result = best_eps.groupby('graph').apply(
#     lambda group: group.assign(
#         diff= group['time'].iloc[0]-group['time'],
#         ratio=group['time'] / group['time'].iloc[0],
#         ratio_reverse=group['time'] / group['time'].iloc[1]
#     )
# ).reset_index(drop=True)
# print(result[result['pattern']=='all'][['graph','pattern','use_all_aux','aux_for_t','time','diff','ratio','ratio_reverse']])

# -------------------------------------------------
# filter epsilon
# -------------------------------------------------
# df = df[(df["epsilon"] == 0.2) | (df["epsilon"] == 0)]
# df = df[((df["epsilon"] == 0.2) & (df["pattern"] != 'tCycles') )
#         | ((df["epsilon"] == 0.5) & (df["pattern"] == 'tCycles'))
#         | (df["epsilon"] == -1)]

# geometric mean helper
gmean = lambda x: np.exp(np.log(x).mean())

# compute geometric mean time per configuration
grouped = (
    df.groupby(["pattern", "use_all_aux", "aux_for_t", "epsilon"])["time"]
      .apply(gmean)
      .reset_index(name="geo_mean_time")
)

# pick epsilon with best geo mean per pattern/config
best_eps = grouped.loc[
    grouped.groupby(["pattern", "use_all_aux", "aux_for_t"])["geo_mean_time"].idxmin()
]

# print results
print(best_eps[["pattern", "use_all_aux", "aux_for_t", "epsilon", "geo_mean_time"]])

# result = best_eps.groupby('pattern').apply(
#     lambda group: group.assign(
#         ratio=group['geo_mean_time'] / group['geo_mean_time'].iloc[0],
#         ratio_reverse=group['geo_mean_time'] / group['geo_mean_time'].iloc[1]
#     )
# ).reset_index(drop=True)

# print(result)

df = df.merge(
    best_eps[["pattern", "use_all_aux", "aux_for_t", "epsilon"]],
    on=["pattern", "use_all_aux", "aux_for_t", "epsilon"],
)


df["struct_time"] = (df.filter(regex=r"^structure_.*$").sum(axis=1, skipna=True))
df["subgraph_time"] = (df.filter(regex=r"^subgraph_.*$").sum(axis=1, skipna=True))
df["iterates"] = (df.filter(regex=r"^iterate_.*$").sum(axis=1, skipna=True))



# -------------------------------------------------
# find metric columns
# -------------------------------------------------
op_cols = [c for c in df.columns if c.startswith("op_")]
iter_cols = [c for c in df.columns if c.startswith("iterate")]

metric_cols = op_cols + iter_cols + ['struct_time'] + ['subgraph_time']  + ["time"]

# keep only non-zero metrics
# metric_cols = [c for c in metric_cols if df_med[c].sum() > 0]

patterns = df["pattern"].unique()

# -------------------------------------------------
# plots per pattern
# -------------------------------------------------
for pattern in patterns:

    dfp = df[df["pattern"] == pattern]

    # =============================================
    # 1) graph plot
    # =============================================
    pivot = dfp.pivot_table(
        index="graph",
        columns="use_all_aux",
        values="time",
        aggfunc="median"
    )

    ax = pivot.plot(kind="bar", figsize=(12,12))

    ax.set_ylabel("time")
    ax.set_title(f"{pattern} — runtime per graph")

    plt.tight_layout()
    plt.savefig(f"{OUTDIR}/{pattern}_graphs.pdf")
    plt.close()

    # =============================================
    # 2) geometric mean metric plot
    # =============================================
    gmeans = {True: {}, False: {}}

    for aux_val in dfp["use_all_aux"].unique():

        df_aux = dfp[dfp["use_all_aux"] == aux_val]

        for col in metric_cols:

            vals = df_aux[col].values
            vals = vals[vals > 0]

            if len(vals) == 0:
                continue

            gmeans[aux_val][col] = np.exp(np.mean(np.log(vals)))

    # separate time
    metrics = [m for m in metric_cols if not m.endswith("time")]
    time_metrics = [m for m in metric_cols if m.endswith("time")]



    vals_true = [gmeans.get(True, {}).get(m, 0) for m in metrics]
    vals_false = [gmeans.get(False, {}).get(m, 0) for m in metrics]

    times_true = [gmeans.get(True, {}).get(m, 0) for m in time_metrics]
    times_false = [gmeans.get(False, {}).get(m, 0) for m in time_metrics]

    # print(time_metrics, times_true)
    
    x = np.arange(len(metrics))
    x2 = np.arange(len(times_true)) + len(metrics)
    width = 0.35

    fig, ax1 = plt.subplots(figsize=(12, 9))

    # operation metrics
    ax1.bar(x - width/2, np.array(vals_false) - np.array(vals_false), width, label="ops (aux=False)")
    ax1.bar(x + width/2, np.array(vals_true) - np.array(vals_false), width, label="ops (aux=True)")

    ax1.set_ylabel("operations (geo mean)")
    # ax1.set_yscale("log")
    ax1.set_xticks(list(x) + list(x2))
    ax1.set_xticklabels(metrics + time_metrics, rotation=60, ha="right")

    # second axis for time
    ax2 = ax1.twinx()
    ax2.bar(x2 - width/2, np.array(times_false) - np.array(times_false), width, label="time (aux=False)")
    ax2.bar(x2 + width/2, np.array(times_true) - np.array(times_false), width, label="time (aux=True)")

    ax2.set_ylabel("time")
    # ax2.set_yscale("log")


    # combine legends
    handles1, labels1 = ax1.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(handles1 + handles2, labels1 + labels2)

    plt.title(f"{pattern} — operation metrics")
    plt.tight_layout()
    plt.savefig(f"{OUTDIR}/{pattern}_metrics.pdf")
    plt.close()

def plot_pattern_boxplot(df, outdir):

    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch

    patterns = ["tCycles", "fCliques", "all"]

    colors = {
        "tCycles": ("#66CCEE", "#4477AA"),
        "fCliques": ("#99CC66", "#228833"),
        "all": ("#EE99AA", "#CC6677"),
    }

    legend_text = {
        "tCycles": ("uLv", "uLv + uHv", "no aux"),
        "fCliques": ("no aux", "cL"),
        "all": ("required aux", "all aux"),
    }

    def darken_color(color, factor=0.7):
            r, g, b = to_rgb(color)
            return (r*factor, g*factor, b*factor)
    fig, axes = plt.subplots(
        1, 3,
        figsize=(10, 6),
        # figsize=(9,4),
        sharey=True,
        gridspec_kw={"wspace": 0}
    )

    for i, (ax, pattern) in enumerate(zip(axes, patterns)):

        dfp = df[df["pattern"] == pattern]


        vals0 = dfp[(dfp["aux_for_t"] == 0) & (dfp["use_all_aux"] == 0)]["time"].values
        if pattern == "all":
            vals0 = dfp[(dfp["aux_for_t"] == 1) & (dfp["use_all_aux"] == 0)]["time"].values


        vals1 = dfp[(dfp["aux_for_t"] == 1) & (dfp["use_all_aux"] == 1)]["time"].values

        if pattern == "tCycles":
            vals2 = dfp[(dfp["aux_for_t"] == 1 ) & (dfp["use_all_aux"] == 0)]["time"].values
            # print(vals2)

        vals =  [vals0,vals1]

        if pattern == "tCycles":
            vals += [vals2]


        bp = ax.boxplot(
            vals,
            widths=0.6,
            patch_artist=True,
            medianprops=dict(color="black", linewidth=2)
        )

        # c0, c1 = colors[pattern]
        c0 = pattern_palette[pattern]
        c1 = darken_color(c0)
        c2 = darken_color(c1)

        bp["boxes"][0].set_facecolor(c0)
        bp["boxes"][1].set_facecolor(c1)

        bp["boxes"][0].set_hatch("")
        bp["boxes"][1].set_hatch("\\")

        if pattern == "tCycles":
            bp["boxes"][2].set_facecolor(c2)
            bp["boxes"][2].set_hatch("|")





        # ax.set_title(pattern)
        ax.set_xlabel(pattern_display[pattern], labelpad=10)

        ax.set_xticks([])          # remove x ticks
        ax.set_yscale("log")       # log runtime
        ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
        ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
        ax.grid(axis="y", linestyle=":", linewidth=0.8)

        # legend per subplot
        handles = [
            Patch(facecolor=c0,edgecolor='black', hatch="", label=legend_text[pattern][0]),
            Patch(facecolor=c1,edgecolor='black', hatch="\\", label=legend_text[pattern][1]),
        ]
        if pattern == "tCycles":
            handles += [
            Patch(facecolor=c2,edgecolor='black', hatch="|", label=legend_text[pattern][2]),
                 
            ]

        ax.legend(handles=handles, fontsize=18)


        # ax.spines["right"].set_visible(True)
        # else:
        #     ax.spines["right"].set_visible(False)

    axes[0].set_ylabel("Time (s)")
    axes[0].set_ylim(top=5000)

    plt.tight_layout()
    print("pattern_boxplot")

    plt.savefig(f"{outdir}/pattern_boxplot.pdf")
    plt.close()

plot_pattern_boxplot(df, "plots")

print("plots written to:", OUTDIR)
