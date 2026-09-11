import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import itertools
from matplotlib import colors as mcolors
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import partition_color,partition_marker,geo_mean,partition_display,partition_linestyle

def print_speedups_new(df_all, df_geo):


    def expand_minus_one(df):
        baseline = df[df["epsilon"] == -1].drop(columns="epsilon")
        others = df[df["epsilon"] != -1]
        join_cols = ["graph"] if "graph" in df.columns else []

        # get unique epsilon combinations to expand into
        epsilons = others[["epsilon"] + join_cols].drop_duplicates()

        if join_cols:
            baseline_expanded = epsilons.merge(
                baseline,
                on=join_cols,
                how="left"
            )
        else:
            # cross join when no graph column exists
            baseline_expanded = epsilons.merge(
                baseline,
                how="cross"
            )

        result = pd.concat([others, baseline_expanded], ignore_index=True)

        return result
        
    def print_speedups(df, metric):
        has_graph = "graph" in df.columns
        index_cols = ["epsilon", "graph"] if has_graph else ["epsilon"]

        

        print(f"\n\t\t\t\t ========= {metric} {"(on graphs)" if has_graph else "(geo mean)"} =========")

        # self-merge within epsilon and graph_group
        # pivot so each partition becomes a column
        # if has_graph:
        #     df = df[df['epsilon']==0.2]
        pivot = df.pivot(
            index=index_cols,
            columns="partition",
            values=metric
        )


        # create speedup columns
        result = pivot.copy()
      
            

        for p1, p2 in itertools.combinations(pivot.columns, 2):
            result[f"{p1}/{p2}"] = pivot[p1] / pivot[p2]
            result[f"{p2}/{p1}"] = pivot[p2] / pivot[p1]

        result["min/time"] = pivot.min(axis=1)
        result["min/by"] = pivot.idxmin(axis=1)

        result.columns = result.columns.str.replace("hindex", "h", regex=False)
        result.columns = result.columns.str.replace("epstab", "et", regex=False)
        result.columns = result.columns.str.replace("_lazy", "l", regex=False)
        result.columns = result.columns.str.replace("_late", "la", regex=False)
        result.columns = result.columns.str.replace("_soft", "s", regex=False)

        # keep only speedup columns
        speedup_df = result[[c for c in result.columns if "/" in c]].reset_index().sort_values(index_cols)

        cols=['epsilon','h2.0/h1.0625','etl/h1.0625','h1.0625/etl','et/etl','etl/et','ets/etl','etl/ets',
            #   'etla/etl',
              'min/by']
        # cols=['epsilon','h2.0/h1.0625','etl/h1.0625','h1.0625/etl',"min/time",'min/by']
        if has_graph:
            cols = ['graph'] + cols
        if has_graph:
            max_val = speedup_df["h1.0625/etl"].max()
            rows = speedup_df[speedup_df["h1.0625/etl"] == max_val]
            print(rows[cols].to_string())

        else:
            print(speedup_df[cols].to_string())

        # group_cols = ["graph"] if has_graph else []
        # speedup_cols = [c for c in result.columns if "/" in c]
        # if group_cols:
        #     print(speedup_df[speedup_df['min/by']!='epstab_lazy'][['graph','etl/et','ets/et','etl/ets', 'min/time', 'min/by' ]].to_string())

        #     min_df = speedup_df.groupby(group_cols)[speedup_cols].min()
        #     min_df["stat"] = "min"

        #     max_df = speedup_df.groupby(group_cols)[speedup_cols].max()
        #     max_df["stat"] = "max"

        #     speedup_min_max = pd.concat([min_df, max_df]).reset_index()
        #     speedup_min_max = speedup_min_max.sort_values(group_cols + ["stat"])

        #     # print(speedup_min_max.to_string())
        # else:
        #     min_df = speedup_df[speedup_cols].min().to_frame().T
        #     min_df["stat"] = "min"

        #     max_df = speedup_df[speedup_cols].max().to_frame().T
        #     max_df["stat"] = "max"

        #     speedup_min_max = pd.concat([min_df, max_df]).reset_index(drop=True)

        #     print(speedup_min_max.to_string())

    def print_speedups_per_eps(df, metric):

        df_lazy = df[df['partition']=='epstab_lazy']
        df_lazy.loc[:, 'partition'] = "el" + df_lazy['epsilon'].astype(str)
        # df_lazy['partition'] = "el" + df_lazy['epsilon'].map(lambda v: str(v))


        # self-merge within epsilon and graph_group
        # pivot so each partition becomes a column
        # if has_graph:
        #     df = df[df['epsilon']==0.2]

        pivot = df_lazy.pivot(
            index='gradual_factor',
            columns="partition",
            values=metric
        )
        # create speedup columns
        result = pivot.copy()

        base='el0.233'
        if base in pivot.columns:
            for p in pivot.columns:
                result[f"{p}/{base}"] = pivot[p] / pivot[base]
                # result[f"{base}/{p}"] = pivot[base] / pivot[p]

        # keep only speedup columns
        speedup_df = result[[c for c in result.columns if "/" in c]].reset_index()
        print(speedup_df.to_string())

    # df_all= expand_minus_one(df_all)
    df_geo= expand_minus_one(df_geo)
    # print_speedups(df_geo, "time_geo_mean")
    print_speedups_per_eps(df_geo, "time_geo_mean")
    # print_speedups(df_geo, "max_update_geo_mean")
    # print_speedups(df_geo, "memory_geo_mean")



    df_all = expand_minus_one(df_all)
    # for eps in df_all['epsilon'].unique():
    #     print_speedups(df_all[df_all['epsilon']==eps], "time")
    # print_speedups(df_all[df_all['epsilon']==0.2], "time")
    # # print_speedups(df_all, "max_update")
    # print_speedups(df_all, "memory")

def plot_time_max_update(df, selected_algo, selected_pattern, outpath, norm="", plot_mem=False):
    metric_to_axis= {
        "time_geo_mean" : "Total Time (s)",
        "max_update_geo_mean" : "Max Update (s)",
        "memory_geo_mean" : "Memory (kB)",
    }
    out_prefix = f"{outpath}/PARTS_{selected_algo}_{selected_pattern}"

    # ------------------------------------------
    # Median over runs
    # ------------------------------------------
    group_cols = ["graph", "algo", "partition", "epsilon", "gradual_factor", "pattern"]
    df_med = df.groupby(group_cols).median().reset_index()
    # ------------------------------------------
    # Select subset
    # ------------------------------------------
    df_sel = df_med[
        (df_med["algo"] == selected_algo) &
        (df_med["pattern"] == selected_pattern)
    ]
    if df_sel.empty:
        return

    agg_metrics = [
        "time",
        "max_update",
        "memory",
    ]
    agg_geo_metrics = [
        "time_geo_mean",
        "max_update_geo_mean",
        "memory_geo_mean",
    ]

    df_geo = df_sel.groupby(["partition", "epsilon", "gradual_factor"]).agg(
        {m: [geo_mean] for m in agg_metrics}
    )
    df_geo.columns = ["_".join(col).rstrip("_") for col in df_geo.columns]
    df_geo = df_geo.reset_index()

    # print_speedups_new(df_sel, df_geo)

    fig, axs = plt.subplots(1, 3, figsize=(15, 5))
    axs = axs.flatten()
    for i, metric in enumerate(agg_geo_metrics):
        ax = axs[i]
        positive_values = []
        for partition in df_geo['partition'].unique():
            if 'hindex' in partition:
                df_g_h = (df_geo[(df_geo["partition"] == partition)])
                value = df_g_h[metric].iloc[0]
                if not np.isfinite(value) or value <= 0:
                    continue
                positive_values.append(value)
                line = ax.axhline(
                    y=value,
                    color=partition_color.get(partition, "black"),
                    linewidth=4,
                    label=partition_display[partition],
                )
                # line.set_dashes((1, 0.5))
                style = partition_linestyle.get(partition)
                line.set_dashes(tuple(style))

            if not partition.startswith("eps"):
                continue

            df_g = (
                df_geo[
                    (df_geo["partition"] == partition)
                ]
                .sort_values("epsilon")
            )
            eps = df_g["epsilon"].values
            # df_g["speedup_to_h"] = df_g_h[metric].iloc[0]/df_g[metric]
            # print(df_g[['graph_group','partition','epsilon',metric,"speedup_to_h"]].to_string())

            if df_g.empty:
                ax.axis("off")
                continue

            mean = df_g[metric]
            valid = np.isfinite(mean) & (mean > 0)
            eps = eps[valid]
            mean = mean[valid]
            if mean.empty:
                continue
            positive_values.extend(mean.tolist())

            color = partition_color.get(partition, "black")
            marker = partition_marker.get(partition, "o")

            # mean line
            ax.plot(
                eps,
                mean,
                color=color,
                linewidth=4,
                marker=marker,
                label=partition_display[partition],
                zorder=2,
            )


        # ax_time.set_ylim(bottom=0)
        # ax_time.set_yscale('log')
        ax.set_xlabel("$\\epsilon$")
        if(metric != "time_geo_mean"):
            if positive_values:
                ax.set_yscale("log")
                # Plain numeric ticks avoid unsupported minus/multiplication
                # glyphs in Computer Modern Sans mathtext tick labels.
                from matplotlib.ticker import FuncFormatter, NullFormatter
                ax.yaxis.set_major_formatter(FuncFormatter(lambda value, _: f"{value:g}"))
                ax.yaxis.set_minor_formatter(NullFormatter())
                low, high = min(positive_values), max(positive_values)
                ax.set_ylim(low / 1.2, high * 1.2)
            else:
                ax.text(0.5, 0.5, "No positive measurements", transform=ax.transAxes,
                        ha="center", va="center", fontsize=12)

        ax.set_ylabel(metric_to_axis[metric])
        ax.grid(True, linestyle='--', alpha=0.5)


        # --- Bring time curves to front ---
        for line in ax.get_lines():
            line.set_zorder(10)  # arbitrary high number

        # ---- TITLE ----
        # ax.set_title(group_name)

        # all_axes_for_legend.extend(ax)

    seen = {}
    for ax in axs:
        h, l = ax.get_legend_handles_labels()
        for handle, label in zip(h, l):
            if label not in seen:
                seen[label] = handle

    fig.legend(
        list(seen.values()),
        list(seen.keys()),
        loc="upper center",
        ncol=6,
        frameon=False,
    )

    fig.tight_layout(rect=[0, 0, 1, 0.87])

    out = f"{out_prefix}.pdf"
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    fig.savefig(out)
    plt.close(fig)

    print(f"Saved {out}")


def main():
    # Load the data from the CSV file
    files = ["results_hhh_eps.csv",
        "results_eps_late.csv",
        "../hindex/results_hhh.csv","results_eps_egst.csv","../hindex/results_egst.csv"
    ]
    dfs =[]
    for input_file in files:
        print(f"Processing {input_file} ...")
        df = pd.read_csv(input_file)
        if df.empty:
            continue
        # Union-schema CSVs contain blank non-applicable statistics, rather
        # than omitting their columns. Preserve the original sentinel values
        # so epsilon-partition rows are not discarded by the filters below.
        for column, default in (("epsilon", -1), ("gradual_factor", -1),
                                ("recompute_mode", 0)):
            if column not in df.columns:
                df[column] = default
            else:
                df[column] = pd.to_numeric(df[column], errors="coerce").fillna(default)
        dfs.append(df)

    if not dfs:
        raise SystemExit("No partition measurements to plot")
    df = pd.concat(dfs, ignore_index=True)
    
   
    df["partition"] = df["partition"] + df["recompute_mode"].map(lambda v: "_soft" if v == 1 else ("_lazy" if v == 2 else ("_late" if v == 3 else "")))
    df = df.drop(columns=["recompute_mode"])


    df["partition"] = df["partition"] + df["gradual_factor"].map(lambda v: str(v) if v > 0 else "")

    # CLEAN df
    df = df[df['gradual_factor'].isin([-1, 2, 1.0625])]
    
    df = df[((df['epsilon']>=0.29) & (df['epsilon']<0.5)) | (df['epsilon']<0) | (df['algo']=='hhh') ]


    # df=df[df['partition']=='hindex']
    print(df["partition"].unique())

    # plot_time_max_update(df, 'hhh', 'all', f"plots")
    # plot_time_max_update(df, 'egst', 'all', f"plots")

    for pat in df["pattern"].unique():
        plot_time_max_update(df, 'hhh', pat, f"plots")
        plot_time_max_update(df, 'egst', pat, f"plots")

    # make_plots(df, 'hhh', 'all', f"plots", "time_geo_mean")
    # make_plots(df, 'hhh', 'all', f"plots", "H_size_mean_geo_mean")

    # for pat in df['pattern'].unique():
        # make_plots(df, 'hhh', pat, f"plots", "time_geo_mean")
        # make_plots(df, 'hhh', pat, f"plots", "max_update_geo_mean")

   
if __name__ == "__main__":
    main()
