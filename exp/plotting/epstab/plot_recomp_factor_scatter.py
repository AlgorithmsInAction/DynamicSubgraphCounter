import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import partition_display,graphs
from util import ComputerModernLogFormatter
import itertools


def print_speedups(df,metric):
    df = df.copy()
    print(f"\n\t\t\t\t ========= {metric} =========")
    # self-merge within epsilon and graph_group
    # pivot so each partition becomes a column
    df['partition'] = df['partition'] + df['recompute_factor'].map(
        lambda v: str(v) )
    df['algo_part'] =  df['algo'] +  "_" +df['partition']
    pivot = df.pivot(
        index=["graph"],
        columns="algo_part",
        values=metric
    )



    pivot.columns = pivot.columns.str.replace("hhh_", "", regex=False)
    pivot.columns = pivot.columns.str.replace("epstab_lazy", "l", regex=False)
    pivot.columns = pivot.columns.str.replace("epstab_lazy", "l", regex=False)
    pivot.columns = pivot.columns.str.replace("epstab_soft", "s", regex=False)
    pivot.columns = pivot.columns.str.replace("epstab", "o", regex=False)


    # create speedup columns
    result = pivot.copy()
    print(result.columns)
    # return

    for base in ['o']: #, 'l', 's']:
        factors =['1.5', '3.0', '5.0', '8.0', '13.0','21.0']
        if base == "l":
            factors = ['1.0', "1.25"] + factors
        for r in factors:
            baseline= base+"2.0"
            ref= base+r
            result[f"{ref}/{baseline}"] = pivot[ref] / pivot[baseline]
            # result[f"{baseline}/{ref}"] = pivot[baseline] / pivot[ref]

    # keep only speedup columns
    speedup_df = result[[c for c in result.columns if "/" in c]].reset_index().sort_values(['graph'])

    def geo_mean_no_nan(series):
        series = series.dropna()          # remove NaN
        series = series[series > 0]       # geometric mean requires positive values
        if len(series) == 0:
            return np.nan
        return np.exp(np.log(series).mean())
    
    # ---- GEO MEAN SINGLE LINE ----
    print(speedup_df.columns)

    # geo_df = speedup_df.apply(geo_mean_no_nan).to_frame().T
    geo_df = (speedup_df.select_dtypes(include="number").apply(geo_mean_no_nan).to_frame().T)
    geo_df.index = ["geo_mean"]

    print("\nGeometric mean speedups: (all instances)")
    col_o = ['o1.5/o2.0', 'o3.0/o2.0', 'o5.0/o2.0', 'o8.0/o2.0',
       'o13.0/o2.0', 'o21.0/o2.0']
    # col_l =['l1.0/l2.0', 'l1.25/l2.0', 'l1.5/l2.0',
    #    'l3.0/l2.0', 'l5.0/l2.0', 'l8.0/l2.0', 'l13.0/l2.0', 'l21.0/l2.0']
    # col_s = ['s1.5/s2.0', 's3.0/s2.0', 's5.0/s2.0', 's8.0/s2.0', 's13.0/s2.0',
    #    's21.0/s2.0']
   
    print(geo_df[col_o].to_string())
    # print(geo_df[col_s].to_string())
    # print(geo_df[col_l].to_string())



def plot_recompute_factor_improvement(df, selected_algo, selected_partition, eps, selected_pattern):
    os.makedirs("plots_RF", exist_ok=True)

    out_file = f"plots_RF/RFACTOR_IMPROVEMENT_{selected_algo}_{selected_partition}_{eps}_{selected_pattern}.pdf"

    # ------------------------------------------
    # Median over runs
    # ------------------------------------------
    group_cols = ["graph", "algo", "partition", "epsilon", "recompute_factor", "pattern"]
    df_med = df.groupby(group_cols)["time"].median().reset_index()

    # ------------------------------------------
    # Filter subset
    # ------------------------------------------
    df_sel = df_med[
        (df_med["algo"] == selected_algo) &
        (df_med["partition"] == selected_partition) &
        (df_med["epsilon"] == eps) &
        (df_med["pattern"] == selected_pattern)
    ].copy()

    graph_groups_dict = {}
    def assign_graph_group(g):
        if g.startswith("g_"):
            group_name = "KRONg"
        elif g.startswith("c_"):
            group_name = "KRONc"
        elif "wiki" in g and "temporal" not in g:
            group_name = "Wiki"
        else:
            group_name = "TMP"
        # Build mapping
        if group_name not in graph_groups_dict:
            graph_groups_dict[group_name] = set()
        graph_groups_dict[group_name].add(g)
        return group_name
    
    df_sel = df_sel.copy()
    df_sel["graph_group"] = df_sel["graph"].apply(assign_graph_group)

    group_style = {
        "KRONg": {
            "color": "#1f77b4",
            "marker": "x"
        },
        "KRONc": {
            "color": "#ff7f0e",
            "marker": "+"
        },
        "Wiki": {
            "color": "#2ca02c",
            "marker": "s"
        },
        "TMP": {
            "color": "#d62728",
            "marker": "*"
        },
    }

    default_style = {
        "color": "black",
        "marker": "x"
    }

    if df_sel.empty:
        print("No data for", selected_pattern, eps)
        return

    # ------------------------------------------
    # Compute ratio vs recompute_factor = 2
    # ------------------------------------------
    baseline = (
        df_sel[df_sel["recompute_factor"] == 2]
        .set_index("graph")["time"]
        .rename("baseline_time")
    )

    df_sel = df_sel.join(baseline, on="graph")

    # remove graphs without baseline
    df_sel = df_sel.dropna(subset=["baseline_time"])

    df_sel["ratio"] = df_sel["time"] / df_sel["baseline_time"]

    # print(df_sel.to_string())

    # ------------------------------------------
    # Plot
    # ------------------------------------------
    plt.figure(figsize=(8, 6))

    graphs = sorted(df_sel["graph"].unique())

    for g in graphs:
        dfg = df_sel[df_sel["graph"] == g]

        if dfg.empty:
            continue

        idx_best = dfg["ratio"].idxmin()

        best_rf = dfg.loc[idx_best, "recompute_factor"]
        best_ratio = dfg.loc[idx_best, "ratio"]
        group = dfg.loc[idx_best, "graph_group"]

        style = group_style.get(group, default_style)

        plt.scatter(
            best_rf,
            best_ratio,
            color=style["color"],
            marker=style["marker"],
            s=80
        )

    for group, style in group_style.items():
        plt.scatter([], [], marker=style["marker"], color=style["color"], label=group)

    plt.legend()
    # reference line
    plt.axhline(1, linestyle=":", linewidth=2, color='gray')
    plt.axvline(2, linestyle=":", linewidth=2, color='gray')

    # plt.yscale("log")
    plt.xscale("log")
    plt.xlabel("recompute_factor")
    plt.ylabel("best time / time(recompute_factor=2)")
    plt.title(f"{selected_pattern}   epsilon={eps}")

    plt.grid(True, which="both", linewidth=0.3)

    plt.tight_layout()
    plt.savefig(out_file)
    plt.close()


def plot_recompute_factor_improvement_all_partitions(df, selected_algo, eps, selected_pattern):
    plt.rcParams.update({ "font.size": 35 })
    
    os.makedirs("plots_RF", exist_ok=True)

    out_file = f"plots_RF/RFACTOR_IMPROVEMENT_{selected_algo}_{eps}_{selected_pattern}.pdf"

    # ------------------------------------------
    # Median over runs
    # ------------------------------------------
    group_cols = ["graph", "algo", "partition", "epsilon", "recompute_factor", "pattern", "num_recomputations"]
    df_med = df.groupby(group_cols)["time"].median().reset_index()

    # ------------------------------------------
    # Filter subset
    # ------------------------------------------
    df_sel = df_med[
        (df_med["algo"] == selected_algo) &
        (df_med["epsilon"] == eps) &
        (df_med["pattern"] == selected_pattern)
    ].copy()


    graph_groups_dict = {}
    def assign_graph_group(g):
        if g.startswith("g_"):
            group_name = "KRONg"
        elif g.startswith("c_"):
            group_name = "KRONc"
        elif "wiki" in g and "temporal" not in g:
            group_name = "Wiki"
        else:
            group_name = "TMP"
        # Build mapping
        if group_name not in graph_groups_dict:
            graph_groups_dict[group_name] = set()
        graph_groups_dict[group_name].add(g)
        return group_name
    
    df_sel = df_sel.copy()
    df_sel["graph_group"] = df_sel["graph"].apply(assign_graph_group)

    group_style = {
        "KRONg": {
            "color": "#E49C39",
            "marker": "d"
        },
        "KRONc": {
            "color": "#6F4C9B",
            "marker": "s"
        },
        "Wiki": {
            "color": "#4E96BC",
            "marker": "8"
        },
        "TMP": {
            "color": "#77B77D",
            "marker": "*"
        },
    }

    default_style = {
        "color": "black",
        "marker": "x"
    }

    if df_sel.empty:
        print("No data for", selected_pattern, eps)
        return

    # ------------------------------------------
    # Compute ratio vs recompute_factor = 2
    # ------------------------------------------
    baseline = (
        df_sel[df_sel["recompute_factor"] == 2]
        .set_index(["graph", "partition"])["time"]
        .rename("baseline_time")
    )

    df_sel = df_sel.join(baseline, on=["graph", "partition"])

    # remove graphs without baseline
    df_sel = df_sel.dropna(subset=["baseline_time"])

    df_sel["ratio"] = df_sel["baseline_time"] / df_sel["time"]
    # print(df_sel.to_string())
    top1 = df_sel.loc[df_sel.groupby('graph')['ratio'].nlargest(1).index.get_level_values(1)]
    print(top1[top1['ratio'] >= top1['ratio'].nlargest(20).min()].sort_values(by='ratio', ascending=False))
    # top2 = df_sel.loc[df_sel.groupby('graph')['ratio'].nlargest(1).index.get_level_values(1)]
    print_speedups(df_sel, 'time')
    # ------------------------------------------
    # Plot
    # ------------------------------------------
    partitions = ['epstab', 'epstab_soft', 'epstab_lazy', 'epstab_late']
    n_partitions = len(partitions)

    fig, axes = plt.subplots(
        2,
        int(n_partitions/2),
        figsize=(n_partitions*4, 12),
        sharey=True
    )

    # If only one partition, axes is not a list
    if n_partitions == 1:
        axes = [axes]


    for ax, part in zip(axes.flatten(), partitions):

        df_part = df_sel[df_sel["partition"] == part]

        graphs = sorted(df_part["graph"].unique())

        for g in graphs:
            dfg = df_part[df_part["graph"] == g]
            if dfg.empty:
                continue


            idx_best = dfg["ratio"].idxmax()
            best_rf = dfg.loc[idx_best, "recompute_factor"]
            best_ratio = dfg.loc[idx_best, "ratio"]
            group = dfg.loc[idx_best, "graph_group"]

            style = group_style.get(group, default_style)

            ax.scatter(
                best_rf,
                best_ratio,
                color=style["color"],
                edgecolors="black",
                linewidth=4,
                marker=style["marker"],
                s=2000,
                zorder=5
            )

          # reference lines
        ax.grid(True, which="both", linewidth=0.3,zorder=1)
        ax.axhline(1, linestyle=":", linewidth=3, color='black',zorder=1)
        ax.axvline(2, linestyle=":", linewidth=3, color='black',zorder=1)

        ax.set_xlim(0.9,25)
        ax.set_xscale("log")
        ax.xaxis.set_major_formatter(ComputerModernLogFormatter())
        ax.xaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
        # ax.set_ylim(bottom=0.99,top=1.06)

        # ax.set_yscale("log")
        ax.set_xlabel("$\\rho$", labelpad=-10)
        ax.set_title(partition_display[part])


    # axes[0].set_ylabel("Max Speedup")
    axes.flatten()[0].set_ylabel("Max Speedup")
    axes.flatten()[2].set_ylabel("Max Speedup")

    # Global legend (only once)
    handles = []
    labels = []

    for group, style in group_style.items():
        h = axes.flatten()[0].scatter([], [],marker=style["marker"],s=1500,edgecolors="black",linewidth=4,color=style["color"],
        )
        handles.append(h)
        labels.append(group.replace("_", "\n"))

    fig.legend(handles, labels, loc="lower center", ncol=4)


    plt.tight_layout(rect=[0, 0.08, 1, 1])
    fig.subplots_adjust(wspace=0.03)
    plt.savefig(out_file)
    plt.close()
    print(f"Saved: {out_file}")



def main():
    # Load the data from the CSV file
    input_file = "results_recomp_factor.csv"
    #------------------------------------------
    # Load CSV
    #------------------------------------------
    df = pd.read_csv(input_file)
    df = df[df['graph'].str.replace('_2m$', '', regex=True).isin(graphs)]

    #------------------------------------------
    # Combine partition and soft_recompute
    #------------------------------------------
    df["partition"] = df["partition"] + df["recompute_mode"].map(lambda v: "_soft" if v == 1 else ("_lazy" if v == 2 else ("_late" if v == 3 else "")))
    # df['partition'] = df['partition'].map(lambda x: partition_config[x]['display_name'])
    print(df["partition"].unique())
    # Then drop the old column
    df = df.drop(columns=["recompute_mode"])

    eps_map = {
        "all":  [0.166], #, 0.2, 0.234],
        "paws":  [0.166], #, 0.2, 0.234],
        "fCycles":  [0.166],# 0.2, 0.234],
        "diamonds":  [0.166],# 0.2, 0.234],
        "triangles": [0.366],# 0.4],
        "tPaths": [0.366],# 0.4]
    }

    # print(df['pattern'].unique())

    plot_recompute_factor_improvement_all_partitions(df, 'hhh', 0.2, 'all')

   
if __name__ == "__main__":
    main()
