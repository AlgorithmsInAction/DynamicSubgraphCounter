import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from util import palette, pattern_hatches, algo_display, pattern_display
from util import ComputerModernLogFormatter
import itertools

# Load CSVs
baseline = pd.read_csv("results_baseline.csv")
baseline_dna = pd.read_csv("results_baseline_dna.csv")
baseline_rapid = pd.read_csv("results_baseline_rapid.csv")
ours = pd.read_csv("results_ours.csv")


# ours = ours[ours['pattern']!='all']
baseline["pattern"] = baseline["pattern"].astype(str) + "s"
baseline["pattern"] = baseline["pattern"].replace("triangles", "tCycles")
baseline['time']=baseline['time']/1000
baseline['graph']=baseline['graph']+"_2m"

baseline_dna["pattern"] = baseline_dna["metric_set"].replace("triangles", "tCycles")

baseline_dna['time']=baseline_dna['overall_total_update_seconds']
baseline_dna['algo']='dna'
baseline_dna['graph'] = baseline_dna['graph'].str.replace(".e", "", regex=False)

# --- RapidFlow baseline ---
baseline_rapid.columns = baseline_rapid.columns.str.strip()
baseline_rapid["time"] = pd.to_numeric(baseline_rapid["time"], errors="coerce")
baseline_rapid["algo"] = "rapidflow"
baseline_rapid["pattern"] = baseline_rapid["pattern"].astype(str) + "s"
baseline_rapid["pattern"] = baseline_rapid["pattern"].replace("triangles", "tCycles")
baseline_rapid['graph']=baseline_rapid['graph']+"_2m"


# Numeric columns
baseline_numeric = baseline.select_dtypes(include="number").columns
baseline_dna_numeric = baseline_dna.select_dtypes(include="number").columns
baseline_rapid_numeric = baseline_rapid.select_dtypes(include="number").columns
ours_numeric = ours.select_dtypes(include="number").columns

# Median aggregation
baseline_med = (
    baseline
    .groupby(["graph", "pattern", "algo"], as_index=False)[baseline_numeric]
    .median()
)

baseline_dna_med = None
if not baseline_dna.empty:
    baseline_dna_med = (
        baseline_dna
        .groupby(["graph", "pattern", "algo"], as_index=False)[baseline_dna_numeric]
        .median()
    )

baseline_rapid_med = (
    baseline_rapid
    .groupby(["graph", "pattern", "algo"], as_index=False)[baseline_rapid_numeric]
    .median()
)

ours = ours[ours["partition"].str.contains("epstab_late|mock|hindex", na=False)]


# ours["algo"] = ours["algo"].where(
#     ours["algo"] != "hhh",
#     ours["algo"] + ours["partition"].map(
#         lambda v: "" if ("epstab_lazy" in v or "mock" in v) else ("l" if "epstab_lazy" in v else "unknown")
#     ),
# )


ours_med = (
    ours
    .groupby(["graph", "pattern", "algo"], as_index=False)[ours_numeric]
    .median()
)

# Add source label
baseline_med["source"] = "baseline"
if baseline_dna_med is not None:
    baseline_dna_med["source"] = "baseline"
baseline_rapid_med["source"] = "baseline"
ours_med["source"] = "ours"

# Align + combine
frames = [baseline_med, baseline_rapid_med, ours_med]
if baseline_dna_med is not None:
    frames.insert(1, baseline_dna_med)
common_cols = list(set.intersection(*(set(item.columns) for item in frames)))
df = pd.concat([item[common_cols] for item in frames])
complete = df.groupby(["algo", "pattern"])["time"].apply(lambda x: x.notna().all())

incomplete = complete[~complete].index
print("Incomplete groups:")
print(incomplete.tolist())

complete = complete[complete].index
df = df.set_index(["algo", "pattern"]).loc[complete].reset_index()

# complete = df.groupby(["algo", "pattern"])["time"].apply(lambda x: x.notna().all())
# complete = complete[complete].index
# df = df.set_index(["algo", "pattern"]).loc[complete].reset_index()


# print("--- Time outs ---")
# print(df[df['time']>18000])
# df["time"]= df["time"].clip(upper=18000)

# print("--- Time ---")

# print(df)
ratio = (
    df.pivot_table(index=["pattern","graph"], columns="algo", values="time")
      .assign(ratio=lambda x: x["graphflow"] / x["hhh"])
)

print(ratio.to_string())



def print_speedups(df,metric):
    # geometric mean helper
    gmean = lambda x: np.exp(np.log(x).mean())

    # compute geometric mean time per configuration
    df = (
        df.groupby(["pattern","algo"])["time"]
        .apply(gmean)
        .reset_index(name="time")
    )
    # df['algo_part'] =  df['algo'] +  "_" +df['partition']
    pivot = df.pivot(
        index=["pattern"],
        columns="algo",
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
    result.columns = result.columns.str.replace("epstab_lazy_", "e", regex=False)
    result.columns = result.columns.str.replace("hindex", "h", regex=False)

    # keep only speedup columns
    speedup_df = result[[c for c in result.columns if "/" in c]].reset_index()

    columns = ['pattern','dna/h','dna/e','rapidflow/h','graphflow/h','graphflow/e','symbi/h']
    print(speedup_df[[column for column in columns if column in speedup_df]].to_string())

print_speedups(df,'time')

# --- BOXPLOT ---

# Unique categories
patterns = ['tCycles','tPaths','paws','fCycles','diamonds','fCliques','all']
algos = [algo for algo in
         ['hhh','egst_high_direct','rapidflow','graphflow','turboflux','symbi','iedyn','dna']
         if algo in set(df['algo'])]


# Prepare data for boxplot
data = []
positions = []
labels = []

pos = 1
width = 0.6

for p in patterns:
    for i, a in enumerate(algos):
        subset = df[(df["pattern"] == p) & (df["algo"] == a)]["time"]
        if len(subset) > 0:
            data.append(subset)
            positions.append(pos)
            labels.append(f"{p}\n{a}")
            pos += 1
    pos += 1  # gap between patterns

# --- BOXPLOT ---

data = []
positions = []
pattern_centers = []

pos = 1
width = 0.9
gap_between_patterns = 0.1

for p in patterns:
    start = pos
    count = 0
    
    for i, a in enumerate(algos):
        subset = df[(df["pattern"] == p) & (df["algo"] == a)]["time"]
        if len(subset) > 0:
            data.append(subset)
            positions.append(pos)
            pos += 1
            count += 1
    
    # center of this pattern group
    if count > 0:
        center = start + (count - 1) / 2
        pattern_centers.append(center)
    
    pos += gap_between_patterns  # gap between patterns

# Plot
fig, ax = plt.subplots(figsize=(15, 7))

bp = ax.boxplot(data, positions=positions, widths=width, patch_artist=True)

for median in bp['medians']:
    median.set_color('black')

box_idx = 0
for p in patterns:
    for a in algos:
        subset = df[(df["pattern"] == p) & (df["algo"] == a)]["time"]
        if len(subset) > 0:
            patch = bp['boxes'][box_idx]
            patch.set_facecolor(palette[a])
            # patch.set_hatch(pattern_hatches[a])
            box_idx += 1

offset = 0
# print(patterns[:-1])
# print(algos)
# print(positions)
for p in patterns[:-1]:
    offset += df[df['pattern']==p]['algo'].nunique()
    x = (positions[offset-1] + positions[offset]) / 2
    ax.axvline(x=x, linestyle='-', color='gray', linewidth=0.5)

# Pattern display names on ticks
ax.set_xticks(pattern_centers)
ax.set_xticklabels([pattern_display[p] for p in patterns],
                   rotation=0, ha="center")

# --- legend (algo-based) ---
legend_handles = [
    Patch(
        facecolor=palette[a],
        edgecolor='black',
        # hatch=pattern_hatches[a],
        label=algo_display[a] + ("(A)" if "hhh" in a else "")
    )
    for a in algos
]

ax.legend(
    handles=legend_handles,
    loc="upper center",
    bbox_to_anchor=(0.5, 1.25),  # move above plot
    ncol=4,
    frameon=False
)


# Labels
ax.set_ylabel("Time(s)")
ax.set_yscale("log")
ax.yaxis.set_major_formatter(ComputerModernLogFormatter())
ax.yaxis.set_minor_formatter(ComputerModernLogFormatter(labelOnlyBase=False))
ax.grid(axis="y", linestyle="--", linewidth=0.5)

plt.tight_layout()
plt.savefig("plots/comp_to_dyn_baselines.pdf")
plt.close()
