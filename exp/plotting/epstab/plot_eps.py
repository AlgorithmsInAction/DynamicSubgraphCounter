import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colors as mcolors
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import metric_palette,metric_markers,metric_linestyle
from util import ComputerModernLogFormatter


def make_plots(df, selected_algo, selected_partition, selected_pattern, outpath, graph_split=True):
    out_prefix = f"{outpath}/EPS_{selected_partition}_{selected_algo}_{selected_pattern}"
    if not os.path.exists(outpath):
        os.makedirs(outpath)
    # ------------------------------------------
    # Median over runs
    # ------------------------------------------
    group_cols = ["graph", "algo", "partition", "epsilon", "pattern"]
    df_med = df.groupby(group_cols).median().reset_index()

    # ------------------------------------------
    # Select subset
    # ------------------------------------------
    df_sel = df_med[
        (df_med["algo"] == selected_algo) &
        (df_med["pattern"] == selected_pattern) &
        (df_med["partition"].isin([selected_partition]))
    ]

    # ------------------------------------------
    # Define graph groups
    # ------------------------------------------
    graph_groups_dict = {}
    def assign_graph_group(g):
        return "all"

        if not graph_split:
            group_name = g
        elif g.startswith("g_"):
            group_name = "KRONg"
        elif g.startswith("c_"):
            group_name = "KRONc"
        elif "wiki" in g and "talk" not in g:
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

    # ------------------------------------------
    # Geometric mean across graphs in each group
    # ------------------------------------------
    def geo_mean(x):
        x = x[x > 0]
        return np.exp(np.log(x).mean())
    def geo_std(x):
        x = x[x > 0]
        return np.exp(np.log(x).std())


    df_sel["operations"] = (df_sel.filter(regex=r"^op_.*$").sum(axis=1, skipna=True))
    df_sel["struct_time"] = (df_sel.filter(regex=r"^structure_.*$").sum(axis=1, skipna=True))
    df_sel["subgraph_time"] = (df_sel.filter(regex=r"^subgraph_.*$").sum(axis=1, skipna=True))
    df_sel["iterates"] = (df_sel.filter(regex=r"^iterate_.*$").sum(axis=1, skipna=True))

    agg_metrics = [
        "time","mean_update","min_update","max_update","99q_update","1q_update",
        "num_recomputations","num_part_changes",
        "changes_mean","changes_max","changes_99q","changes_1q",
        "H_size_mean","H_size_max","H_size_99q","H_size_1q",
        "memory",
        "iterate_neighbor","iterate_h",        
        "structure_addTime","structure_delTime","structure_toHighTime","structure_toLowTime","subgraph_addTime","subgraph_delTime",
        "partition_time","operations","struct_time","subgraph_time","iterates"
    ]

    agg_metrics += [m for m in df_sel.columns if m.startswith("op_")]


    df_geo = df_sel.groupby(["partition", "epsilon", "graph_group"]).agg(
        {m: [geo_mean, geo_std] for m in agg_metrics}
    )
    df_geo.columns = ["_".join(col).rstrip("_") for col in df_geo.columns]
    df_geo = df_geo.reset_index()

    # ------------------------------------------
    # Helper function for plotting
    # ------------------------------------------
    def plot_metric_group(df, metrics, title, output_file,  min_metric=None, xline=None, graph_split=True):
        graph_groups = df["graph_group"].unique()
        rows = len(graph_groups)
        fig, axes = plt.subplots(rows, 1, figsize=(14, 4*rows), sharey=False)
        if rows == 1:
            axes = np.array([axes])

        for r, group_name in enumerate(graph_groups):
            df_group = df[df["graph_group"] == group_name].reset_index()

            ax = axes[r]
            dfp = df_group[df_group["partition"] == selected_partition].sort_values("epsilon")
            eps = dfp["epsilon"].values


            for m in metrics:
                mean_col = f"{m}_geo_mean"
                # std_col = f"{m}_geo_std"
                if mean_col not in dfp.columns:
                    continue
                mean_vals = dfp[mean_col].values
                # std_vals = dfp[std_col].values
                ax.plot(eps, mean_vals, label=m)
                # ax.fill_between(eps, mean_vals - std_vals, mean_vals + std_vals, alpha=0.2)

            # Vertical lines
            # print(dfp.columns)
            y_transform = ax.get_xaxis_transform()
            if min_metric is not None and f"{min_metric}_geo_mean" in dfp.columns:
                # print( f"\n\n {min_metric} \n")
                # dfp[f"{min_metric}_geo_mean"] = dfp[f"{min_metric}_geo_mean"].fillna(float('inf'))
                # print(dfp[f"{min_metric}_geo_mean"])
                # print(dfp[f"{min_metric}_geo_mean"].idxmin())
                idx_min = dfp[f"{min_metric}_geo_mean"].dropna().idxmin()
                eps_min = dfp.loc[idx_min, "epsilon"]
                ax.axvline(eps_min, color="red", linestyle="--", linewidth=1)
                ax.text(eps_min, -0.12, f"{eps_min:.3g}", color="red",
                        ha="center", rotation=90, transform=y_transform)
            if xline is not None:
                ax.axvline(xline, color="black", linestyle=":", linewidth=1.5)
                ax.text(xline, -0.12, f"{xline:.3g}", color="black",
                        ha="center", rotation=90, transform=y_transform)

            graph_names_str = format_graph_list(graph_groups_dict[group_name], per_line=5)
            ax.set_title(f"{title} — {selected_partition}\n[{graph_names_str}]")
            ax.set_xlabel("epsilon")
            ax.set_yscale("log")
            ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
            ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
            ax.grid(True, which="both", linewidth=0.3)
            ax.legend(fontsize=7)

        fig.tight_layout()
        fig.savefig(output_file)
        plt.close(fig)

    # ------------------------------------------
    # Metric groups
    # ------------------------------------------
    time_metrics = ["time","mean_update","min_update","max_update","99q_update","1q_update"]
    change_metrics = ["num_recomputations","num_part_changes","changes_mean","changes_max","changes_99q","changes_1q"]
    size_metrics = ["H_size_mean","H_size_max","H_size_99q","H_size_1q"]

    # ------------------------------------------
    # Create plots
    # ------------------------------------------
    # plot_metric_group(df_geo,
    #                   time_metrics, "Time Metrics", f"{out_prefix}_time.pdf", min_metric="time", graph_split=graph_split)
    # plot_metric_group(df_geo,
    #                   change_metrics, "Partition Changes", f"{out_prefix}_partition_changes.pdf", graph_split=graph_split)
    # plot_metric_group(df_geo,
    #                   size_metrics, "Partition Size", f"{out_prefix}_partition_size.pdf", graph_split=graph_split)
    
    
    # print("Saved PDFs:")
    # print(f"  {out_prefix}_time.pdf")
    # print(f"  {out_prefix}_partition_changes.pdf")
    # print(f"  {out_prefix}_partition_size.pdf")

    # plot_epsilon_multi_axis( df_geo, selected_partition, out_prefix, graph_groups_dict)
    plot_epsilon_multi_axis_2x2( df_geo, selected_partition, out_prefix, graph_groups_dict)
    

def format_graph_list(graph_list, per_line=5):
    """
    Returns a string with graph names, breaking lines every `per_line` names.
    """
    graph_list = list(graph_list) 
    lines = []
    for i in range(0, len(graph_list), per_line):
        lines.append(", ".join(graph_list[i:i+per_line]))
    return "\n".join(lines)

def plot_epsilon_multi_axis(df, selected_partition, out_prefix, graph_groups_dict):
    # df['operations_geo_mean'] = df["op_vLV_geo_mean"]+df["op_uLv_geo_mean"]+df["op_t_geo_mean"]+df["op_uLLv_geo_mean"]+df["op_cLV_geo_mean"]+df["op_pLL_geo_mean"]+df["op_uHv_geo_mean"]+df["op_cL_geo_mean"]
    df["operations_geo_mean"] = df.filter(regex=r"^op_.*geo_mean$").sum(axis=1, skipna=True)

    df['struct_time_geo_mean'] = df["structure_addTime_geo_mean"]+df["structure_delTime_geo_mean"]+df["structure_toHighTime_geo_mean"]+df["structure_toLowTime_geo_mean"]
    df['subgraph_time_geo_mean'] =df["subgraph_addTime_geo_mean"]+df["subgraph_delTime_geo_mean"]
    df["iterates_geo_mean"] = df.filter(regex=r"^iterate_.*geo_mean$").sum(axis=1, skipna=True)

    # df['iterates_geo_mean'] =df["iterate_neighbor_geo_mean"]+df["iterate_h_geo_mean"]
    df['time_sum_geo_mean'] =df["struct_time_geo_mean"]+df["subgraph_time_geo_mean"]+df["partition_time_geo_mean"]


    metrics_time = [
        ("time", "Time"),
        # ("time_sum", "Summed Time"),
        ("struct_time", "Structure Time"),
        ("subgraph_time", "Counting Time"),
        ("partition_time", "Partition Time"),
    ]

    metrics_left = [
        # ("H_size_mean", "H size (mean)"),
        # ("memory", "Memory"),
        # ("dtlb_miss_percent", "DTLB miss %"),
        # ("num_part_changes", "\\#Part Changes")
    ]
    metrics_right = [
        ("iterate_neighbor", "Iterate neighbor"),
        ("iterate_h", "Iterate h"),
        ("iterates", "Iterate h+neighbor"),
        ("operations", "\\#Structure Count Changes"),
        ("num_part_changes", "\\#Part Changes")

    ]

    metric_colors = {
    "time": "red",

    "struct_time": "tab:blue",
    "operations": "tab:blue",

    "subgraph_time": "tab:green",
    "iterates": "tab:green",

    "partition_time": "tab:orange",
    "num_part_changes": "tab:orange",

    "iterate_neighbor": "tab:brown",
    "iterate_h": "tab:gray",

    "dtlb_miss_percent": "tab:pink",
}

    for group_name in df["graph_group"].unique():
        df_g = (
            df[(df["graph_group"] == group_name) &
               (df["partition"] == selected_partition)]
            .sort_values("epsilon")
        )

        if df_g.empty:
            continue

        eps = df_g["epsilon"].values

        fig, ax_time = plt.subplots(figsize=(14, 6))

        # ---- LEFT AXES ----
        axes_left = [ax_time]
        colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]

        for i, (metric, label) in enumerate(metrics_time):
            col = f"{metric}_geo_mean"
            if col not in df_g.columns:
                continue
            ax_time.plot(
                eps,
                df_g[col],
                color=metric_colors.get(metric, "black"),
                linestyle="-",
                marker='*',
                label=label,
            )
        ax_time.set_yscale("log")
        ax_time.yaxis.set_major_formatter(ComputerModernLogFormatter())
        ax_time.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
        # ax_time.set_ylim(bottom=0)
        ax_time.set_ylabel("Time (s)")


        ax_time.set_xlabel("epsilon")


        for i, (metric, label) in enumerate(metrics_left):
            col = f"{metric}_geo_mean"
            if col not in df_g.columns:
                continue

            ax = ax_time.twinx()

            # move this axis to the RIGHT, outward
            ax.spines["right"].set_position(("outward", 60 * (i + 1)))
            ax.spines["left"].set_visible(False)
       
            ax.yaxis.set_label_position("right")
            ax.yaxis.tick_right()
            ax.yaxis.get_offset_text().set_x(1.1 + 0.1* i)  # tune this


            ax.plot(
                eps,
                df_g[col],
                linestyle="--",
                marker='^',
                color=metric_colors.get(metric, "black"),
                label=label,
            )

            if metric == "num_part_changes":
                np = df_g[col].values

                ymin, ymax = ax_time.get_ylim()
                # bottom_val = ((0 - ymin) / (ymax - ymin))*(ymax-ymin)


                pt_scale = df_g["partition_time_geo_mean"].iloc[0]/ ymax
                top_val = df_g["num_part_changes_geo_mean"].iloc[0]/ pt_scale

                # Set ylim so anchors align
                ax.set_ylim(top=top_val*0.5)


            # ax.set_ylim(bottom=0)
            ax.set_ylabel(label, color=metric_colors.get(metric, "black"))
            ax.tick_params(axis="y", color=metric_colors.get(metric, "black"))
            ax.set_yscale("log")
            ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
            ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))

            axes_left.append(ax) 

        # ---- RIGHT AXIS (shared) ----
        ax_r = ax_time.twinx()
        for i, (metric, label) in enumerate(metrics_right):
            col = f"{metric}_geo_mean"
            if col not in df_g.columns:
                continue
            ax_r.plot(
                eps,
                df_g[col],
                linestyle="--",
                color=metric_colors.get(metric, "black"),
                marker='+',
                label=label,
            )

        # ax_r.set_ylim(bottom=0)
        ax_r.set_ylabel("Iterations")
        ax_r.set_yscale("log")
        ax_r.yaxis.set_major_formatter(ComputerModernLogFormatter())
        ax_r.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
        # ax_r.set_ylim(bottom=1e6)

        # ---- LEGEND ----
        lines = []
        labels = []
        for ax in axes_left + [ax_r]:
            l, lab = ax.get_legend_handles_labels()
            lines += l
            labels += lab

        ax_time.legend(lines, labels, fontsize=8, loc="best")

        graph_names = format_graph_list(graph_groups_dict[group_name], per_line=5)
        ax_time.set_title(
            f"Epsilon sweep — {group_name}\n[{graph_names}]"
        )

        ax_time.grid(True, which="both", linewidth=0.3)
        fig.tight_layout()

        out = f"{out_prefix}_epsilon_multi_{group_name}.pdf"
        fig.savefig(out)
        plt.close(fig)

        print(f"Saved {out}")

def plot_epsilon_multi_axis_2x2(df, selected_partition, out_prefix, graph_groups_dict):

    # ---- METRIC GROUPS ----
    metrics_time = [
        ("time", "Total Time"),
        ("struct_time", "Structure Time"),
        ("subgraph_time", "Counting Time"),
        ("partition_time", "Partition Time"),
    ]

    metrics_right = [
        ("iterates", "$\\sum$(H/Neighbors Traversals)"),
        ("operations", "\\#Structure Updates"),
        ("num_part_changes", "\\#Partition Changes"),
    ]

    # ---- HELPERS ----
    def chunked(lst, n):
        for i in range(0, len(lst), n):
            yield lst[i:i + n]

    graph_groups = list(df["graph_group"].unique())

    # ---- MAIN LOOP (2x2 FIGURES) ----
    for batch_idx, group_batch in enumerate(chunked(graph_groups, 4)):

        fig, axs = plt.subplots(1, 1, figsize=(14, 6))
        # axs = axs.flatten()

        all_axes_for_legend = []

        group_name=group_batch[0]
        ax_time=axs

        # for ax_time, group_name in zip(axs, group_batch):

        df_g = (
            df[
                (df["graph_group"] == group_name) &
                (df["partition"] == selected_partition)
            ]
            .sort_values("epsilon")
        )

        if df_g.empty:
            ax_time.axis("off")
            continue

        eps = df_g["epsilon"].values

        # ---- TIME AXES (LEFT, BASE) ----
        axes_left = [ax_time]

        for metric, label in metrics_time:
            mean_col = f"{metric}_geo_mean"
            std_col  = f"{metric}_geo_std"

            if mean_col not in df_g.columns or std_col not in df_g.columns:
                continue

            color = metric_palette.get(metric, "black")

            mean = df_g[mean_col]
            std  = df_g[std_col]
            # print(metric,mean-std)


            # # shaded area FIRST (so the line stays on top)
            # ax_time.fill_between(
            #     eps,
            #     mean / std,
            #     mean * std,
            #     color=color,
            #     alpha=0.2,
            #     linewidth=0,
            #     zorder=1,
            # )

            # mean line
            ax_time.plot(
                eps,
                mean,
                color=color,
                marker=metric_markers.get(metric, "o"),
                linestyle=(0, metric_linestyle.get(metric, "-")),
                label=label,
                zorder=2,
            )


        # ax_time.set_ylim(bottom=0)
        ax_time.set_yscale('log')
        ax_time.yaxis.set_major_formatter(ComputerModernLogFormatter())
        ax_time.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
        ax_time.set_xlabel("$\\epsilon$")
        ax_time.set_ylabel("Total Time (s)")
        ax_time.grid(True, linestyle='--', alpha=0.5)


        # ---- MIN TIME MARKER ----
        time_col = "time_geo_mean"
        if time_col in df_g.columns:

            idx_min = df_g[time_col].idxmin()
            eps_min = df_g.loc[idx_min, "epsilon"]

            ax_time.axvline(
                eps_min,
                color=metric_palette.get("time", "black"),
                linestyle=":",
                alpha=0.9,
            )
            print(out_prefix, eps_min, df_g.loc[idx_min, "time_geo_mean"])

            # ax_time.text(
            #     eps_min,
            #     ax_time.get_ylim()[1] * 0.95,
            #     f"{eps_min:.4g}",
            #     # f"$\\epsilon$={eps_min:.3g}",
            #     color=metric_palette.get("time", "black"),
            #     rotation=90,
            #     va="top",
            #     ha="right",
            #     fontsize=14,
            #     # backgroundcolor="white",
            # )

        # --- Bring time curves to front ---
        for line in ax_time.get_lines():
            line.set_zorder(10)  # arbitrary high number

        # ---- RIGHT AXIS ----
        ax_r = ax_time.twinx()
        ax_r.set_zorder(ax_time.get_zorder() + 1)

        for metric, label in metrics_right:
            col = f"{metric}_geo_mean"
            if col not in df_g.columns:
                continue

            ax_r.plot(
                eps,
                df_g[col],
                color=metric_palette.get(metric, "black"),
                marker=metric_markers.get(metric, 'o'),
                linestyle=(0, metric_linestyle.get(metric, '-')),
                label=label,
            )

        # ax_r.set_ylim(bottom=0)
        ax_r.set_yscale('log')
        ax_r.yaxis.set_major_formatter(ComputerModernLogFormatter())
        ax_r.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
        ax_r.set_ylabel("Algorithmic Operations")

        ax_r.set_zorder(5)  # lower than time lines
        ax_time.set_zorder(ax_r.get_zorder() + 1)  # time axis above right axis
        ax_time.patch.set_visible(False)           # make background transparent so twin axes don’t cover it


        # ---- TITLE ----
        # ax_time.set_title(group_name)

        all_axes_for_legend.extend(axes_left + [ax_r])

        # ---- TURN OFF UNUSED SUBPLOTS ----
        # for ax in [len(group_batch):]:
        # axs.axis("off")

        # ---- SHARED LEGEND ----
        # ---- SINGLE SHARED LEGEND (DEDUPLICATED) ----
        legend_order = [
            "Structure Time",
            "\\#Structure Updates",
            "Counting Time",
            "$\\sum$(H/Neighbors Traversals)",
            "Partition Time",
            "\\#Partition Changes",
            "Total Time",
        ]
        seen = {}
        for ax in all_axes_for_legend:
            h, l = ax.get_legend_handles_labels()
            for handle, label in zip(h, l):
                if label not in seen:
                    seen[label] = handle

         

        ordered_labels = [
            label for label in legend_order if label in seen
        ]

        ordered_handles = [
            seen[label] for label in ordered_labels
        ]

        # fig.legend(
        #     ordered_handles,
        #     ordered_labels,
        #     loc="upper center",
        #     ncol=1,
        #     frameon=False,
        # )
        fig.legend(
            ordered_handles,
            ordered_labels,
            loc="upper left",
            bbox_to_anchor=(0.6, 0.9),
            ncol=1,
            frameon=False,
        )


        # fig.suptitle(
        #     f"Epsilon sweep — partition={selected_partition}",
        #     fontsize=14,
        # )

        fig.tight_layout(rect=[0, 0, 0.6, 1])

        out = f"{out_prefix}_grid.pdf"
        fig.savefig(out)
        plt.close(fig)

        print(f"Saved {out}")


def main():
    # Load the data from the CSV file
    input_file = "results_hhh_eps.csv"
    # input_file = "results_eps_egst.csv"
    # input_file = "results_egst_hindex.csv"


    print(f"Processing {input_file} ...")
    #------------------------------------------
    # Load CSV
    #------------------------------------------
    df = pd.read_csv(input_file)
    # print(df["partition"].unique())


    #------------------------------------------
    # Combine partition and soft_recompute
    #------------------------------------------
    df["partition"] = df["partition"] + df["recompute_mode"].map(lambda v: "_soft" if v == 1 else ("_lazy" if v == 2 else ""))
    df = df.drop(columns=["recompute_mode"])

    print(df["partition"].unique())

    for partition in df["partition"].unique(): 

        for pat in df['pattern'].unique():
            for algo in df['algo'].unique():
                make_plots(df, algo, partition, pat, f"plots_{partition}")

   
if __name__ == "__main__":
    main()
