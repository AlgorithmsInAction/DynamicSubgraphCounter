# SubgraphCounter: Dynamic, configurable subgraph counting for undirected triangle and 4-vertex patterns

SubgraphCounter dynamically maintains counts of small subgraphs (triangles, 3-paths, claws, paws, 4-cycles, diamonds, 4-cliques) under edge updates. Multiple dynamic algorithms and vertex-partitioning strategies are provided, with optional tuning parameters for performance/accuracy tradeoffs.

This directory reproduces the experiments in **“Fully Dynamic Triangle and
4-Vertex Subgraph Counting: From Theory to Practice and Back Again”**.

## Reproduce from a ZIP on a clean machine

Nothing from this project needs to be installed on the host. The host only
needs a 64-bit Linux installation, Docker, Python 3, `unzip`, network access,
sufficient disk space, and sufficient NUMA-local RAM. Docker installs all
compilers, libraries, Python packages, and data-preparation tools inside the
image.

From the directory containing the supplied archive, extract it and enter the
project directory. Adjust the archive or directory name if necessary.

```bash
unzip subgraph-counting.zip
cd subgraph-counting
chmod +x exp/run.sh exp/scripts/*.sh exp/experiments/*.sh
```

Compile the application and all required helper tools:

```bash
docker build -f exp/Dockerfile -t subgraph-counting-exp .
```

`easyCompile` downloads public [oaqc](https://github.com/schochastics/oaqc) and
[ESCAPE](https://bitbucket.org/seshadhri/escape) sources (for the OB/ESCAPE
baselines) into the Git-ignored `deps/` directory. Further the docker container downloads [AlgoraCore](https://gitlab.com/libalgora/AlgoraCore),
[AlgoraDyn](https://gitlab.com/libalgora/AlgoraDyn), RapidFlow,
ContinuousSubgraphMatching, and DNA, installs the build dependencies, and compiles the
main benchmark and all three reference applications plus `graph_cleaner` and
`temporal_to_updates`. The run script also invokes this build command
automatically; Docker reuses the cached image when nothing changed.

The image fetches Ubuntu indexes by content hash and retries cleanly to tolerate
mirrors or HTTP caches that are briefly out of sync. If all five attempts fail,
rerun the same command after the mirror has synchronized; do not bypass APT's
signature or checksum verification.

Choose either the single-command reproduction (estimated runtime of 11.3 days on a machine with 16 L3-cache disjoint CPUs and 1.48 TB RAM):

```bash
./exp/run.sh --all
```

or run the suites separately in the recommended order:

```bash
# 1. Main engineering-set experiments (Section 5.2.1 and Figures 5.1--5.5)
# Estimated runtime of 5.6 days on a machine with 16 L3-cache disjoint CPUs and 1.48 TB RAM
./exp/run.sh --engineering_set_main

# 2. Full-set experiment (Figure 5.6)
# Estimated runtime of 4.5 days on a machine with 16 L3-cache disjoint CPUs and 1.48 TB RAM
./exp/run.sh --full_set

# 3. Optional appendix engineering-set experiments (Figures A.1--A.5)
# Estimated runtime of 1.2 days on a machine with 16 L3-cache disjoint CPUs and 1.48 TB RAM
./exp/run.sh --engineering_set_appendix
```

Each command downloads and prepares only missing data, resumes completed jobs,
checks CPU/NUMA memory capacity, runs experiments sequentially, and places final
plots in `exp/plots/`. Before committing to a long run, inspect the detected
CPUs and estimates:

```bash
./exp/run.sh --list
./exp/run.sh --all --estimate
```


## Runtime estimates

The runner prints an estimated runtime before data preparation. `--estimate`
prints it without building, downloading, or running anything. These projections
are based on previous experiment measurements.
They are not guaranteed runtimes: filesystem and network I/O, host load,
hardware differences, preprocessing, plotting, scheduling overhead, and other
unmodeled factors can change the actual duration. Values are expressed as
CPU-hours and as estimated wall time for the number of CPUs selected by
`--cpus`. With 16 CPUs, the CLI uses the memory-aware target-machine model
below; other CPU counts use the idealized CPU-hours divided by worker count.

### Single-worker estimates

“Single worker” means executing the complete experiment grid serially.

| Experiment | Estimated time | Estimated peak memory |
|---|---:|---:|
| Section 5.2.1 HHH optimization | 158 h / 6.6 days | 664.2 GiB |
| Figure 5.1 | 21 h / 0.9 days | 14.6 GiB |
| Figure 5.2 | 120 h / 5.0 days | 14.6 GiB |
| Figure 5.3 | 132 h / 5.5 days | 4.1 GiB |
| Figure 5.4 | 94 h / 3.9 days | 11.2 GiB |
| Figure 5.5, including references | 900 h / 37.5 days | 4.2 GiB (native only) |
| Figure 5.6 | 1,480 h / 61.7 days | 271.6 GiB |
| Figure A.1 | 9 h / 0.4 days | 3.8 GiB |
| Figure A.2 | 38 h / 1.6 days | 5.1 GiB |
| Figure A.3 | 71 h / 2.9 days | 109.4 GiB |
| Figure A.4 | 189 h / 7.9 days | 14.6 GiB |
| Figure A.5 | 145 h / 6.0 days | 4.0 GiB |

| Mode | Estimated time | Estimated peak memory |
|---|---:|---:|
| `--engineering_set_main` | 1,425 h / 59.4 days | 664.2 GiB |
| `--engineering_set_appendix` | 452 h / 18.8 days | 109.4 GiB |
| `--full_set` | 1,480 h / 61.7 days | 271.6 GiB |
| `--all` | 3,356 h / 139.8 days | 664.2 GiB |

### Target-machine estimates: 16 CPUs and 1.48 TB RAM

This model uses 16 workers on **1.48 TB decimal RAM**, split evenly over two
NUMA nodes: eight workers and 689.2 GiB per node. The memory column is the
modeled peak sum of simultaneously active job reservations, not `16 ×` the
largest job.

The wall time **includes worker idle time** when a NUMA node cannot fit another
pending job, as well as the tail caused by jobs finishing at different times.
This is why Section 5.2.1 increases from an ideal 9.9 h to 49.1 h and Figure 5.6
from an ideal 92.5 h to 107.9 h.

| Experiment | Estimated wall time | Estimated peak memory |
|---|---:|---:|
| Section 5.2.1 HHH optimization | 49 h / 2.0 days | 1,376.4 GiB |
| Figure 5.1 | 1 h / 0.1 days | 109.2 GiB |
| Figure 5.2 | 8 h / 0.3 days | 109.2 GiB |
| Figure 5.3 | 8 h / 0.3 days | 34.2 GiB |
| Figure 5.4 | 7 h / 0.3 days | 42.0 GiB |
| Figure 5.5, including references | 61 h / 2.5 days | 60.5 GiB (native only) |
| Figure 5.6 | 108 h / 4.5 days | 1,059.0 GiB |
| Figure A.1 | 1 h / 0.0 days | 22.9 GiB |
| Figure A.2 | 2 h / 0.1 days | 58.0 GiB |
| Figure A.3 | 5 h / 0.2 days | 838.2 GiB |
| Figure A.4 | 12 h / 0.5 days | 104.6 GiB |
| Figure A.5 | 9 h / 0.4 days | 34.8 GiB |

| Mode | Estimated wall time | Estimated peak memory |
|---|---:|---:|
| `--engineering_set_main` | 134 h / 5.6 days | 1,376.4 GiB |
| `--engineering_set_appendix` | 29 h / 1.2 days | 838.2 GiB |
| `--full_set` | 108 h / 4.5 days | 1,059.0 GiB |
| `--all` | 271 h / 11.3 days | 1,376.4 GiB |

## What runs

| Suite | Subscript | Paper result |
|---|---|---|
| engineering main | `sec_5_2_1_hhh_optimizations.sh` | base vs. high-anchor HHH optimization |
| engineering main | `fig_5_1_hhh_epsilon.sh` | HHH epsilon/runtime breakdown |
| engineering main | `fig_5_2_hhh_partitions.sh` | HHH partition study |
| engineering main | `fig_5_3_egst_partitions.sh` | EGST partition study |
| engineering main | `fig_5_4_static_baselines.sh` | OB/ESCAPE comparison |
| engineering main | `fig_5_5_dynamic_baselines.sh` | optimized HHH/EGST, Graphflow, SymBi, IEDyn, TurboFlux, RapidFlow, and DNA |
| full set | `fig_5_6_full_set.sh` | all 56 full streams |
| engineering appendix | `fig_a_1_extra_aux.sh` | extra auxiliary structures |
| engineering appendix | `fig_a_2_rebalance_factor.sh` | rebalance factor rho |
| engineering appendix | `fig_a_3_egst_optimizations.sh` | EGST H/D optimizations |
| engineering appendix | `fig_a_4_hhh_single_patterns.sh` | single-pattern HHH tuning |
| engineering appendix | `fig_a_5_egst_single_patterns.sh` | single-pattern EGST tuning |

Every subscript can also be called directly; it accepts the paths and switches
passed by `run.py`.  The main runner executes subscripts sequentially and prints
`[x/y done]`.  Within one experiment it schedules jobs over the selected CPUs
and shows `[runs completed/total; percent]` on one updating terminal line.
Redirected output is limited to 5% updates, while failures are always printed.
Successful non-empty `ssf_*.txt` files are resume markers, so interrupted
invocations continue where they left off. Logs and the exact command manifest
are retained per experiment.

Every selected experiment follows the same four stages:

1. download and prepare missing graph data;
2. execute the experiment's run grid;
3. merge `ssf_*.txt` summaries into `results.csv` by column name, with one
   combined header and blank values for non-applicable statistics. Different
   algorithm/partition schemas are supported; unused `noentry` padding is
   discarded. Raw per-run files remain unchanged;
4. run the vendored original paper plotting program and copy its final PDF into
   `exp/plots/`.

Collection is deterministic and is repeated cheaply after a resumed run. The
compact `exp/resources/memory_reservations.csv` contains only per-job memory
estimates based on previous measurements; it contains no experiment results
and is never substituted for newly measured output.
Figure 5.5 keeps its differently shaped reference summaries in
`results_csm.csv`, `results_rapidflow.csv`, and `results_dna.csv` rather than mixing incompatible
headers into `results.csv`.

Figure 5.5 builds and reruns three upstream reference codebases. The pinned
revisions are recorded as Docker build arguments in `exp/Dockerfile`:

* [ContinuousSubgraphMatching](https://github.com/RapidsAtHKUST/ContinuousSubgraphMatching)
  supplies Graphflow, SymBi, IEDyn, and TurboFlux. Graphflow and SymBi run on
  all six single patterns; IEDyn and TurboFlux run on the length-3 path, matching
  the supplied paper-result grid.
* [RapidFlow](https://github.com/shixuansun/RapidFlow) runs on all six patterns.
* [DNA](https://github.com/AlgorithmsInAction/subgraphcounting_DNA) runs
  triangle-only and all-pattern maintenance on the same signed-update streams.
  It is compiled once with JDK 17. The harness directly invokes its Java runner
  in `aggregate-only` timing mode, with a five-hour timeout, one active CPU,
  serial garbage collection, and the same CPU/NUMA binding as other jobs.
  Individual four-node patterns are not timed separately by DNA.

Each reference is run three times on every engineering graph. Its reported
runtime and status are normalized into the same resumable `ssf_*.txt` layout;
reference memory is not measured. Nonzero exits and upstream timeouts are
recorded as completed outcomes for CSM/RapidFlow instead of aborting the grid.
DNA timeouts produce completed partial observations; DNA launch failures remain
retryable. DNA's own aggregate CSV is preserved per run and collected into
`results_dna.csv`. Plots use these newly measured DNA timings, never the old
bundled baseline CSV. Existing completed native/CSM/RapidFlow jobs are reused
when resuming Figure 5.5; only missing DNA jobs need to run.

## Data download and preparation

The self-contained downloader in `exp/scripts/get_data.sh` and preparation
helpers in `exp/tools/` implement the paper's data recipes:

* **DyReach** (`dyreach.taa.univie.ac.at`) supplies the constant-size and
  growing Kronecker streams and the dynamic KONECT Wikipedia streams.
* **SNAP** supplies its timestamped interaction streams.
* **Network Repository** supplies the remaining timestamped streams.

Only missing downloads or generated files are processed.  Downloads first go
to `data/raw/*.part` and are atomically renamed when complete.  Preparation:

1. decompresses each source;
2. gives timestamped edges the paper's 14-day lifetime;
3. canonicalizes undirected endpoints and removes self-loops, duplicate
   insertions, and deletions of absent edges;
4. derives the first-2,000,000 and first-150,000 update streams;
5. creates the vertex/update representation needed by CSM baselines.

The resulting layout is:

```text
exp/data/
  raw/             downloaded and unpacked source files
  full/            complete cleaned `.e` streams
  2m/              engineering streams
  150k/            static-baseline streams
  2m_csm_format/   `.vertices` and `.updates` files
```

Data already present in `exp/data/` is reused before downloading anything.
The complete corpus is large; `--engineering_set_main` or
`--engineering_set_appendix` downloads only the 24 engineering graphs.

The immutable inputs needed by the harness itself are bundled under `exp/`:

```text
exp/tools/                 data-conversion source code compiled by Docker
exp/resources/csm/pattern/ query graphs used by the reference implementations
exp/resources/memory_reservations.csv
                           per-job estimates based on previous measurements
```

These files deliberately live outside `exp/data/` and `exp/results/`, so a
normal run never overwrites them. Before building Docker, `run.sh` validates
only the resources required by the selected experiment and mounts the resource
directory read-only. CSM query graphs are therefore required for Figure 5.5,
but not for Figure 5.1 or other experiments that do not run the references.
The application source and `easyCompile` at the project root are still required
to build SubgraphCounter; only the legacy experiment/data scripts have been
made unnecessary.

## Outputs

Section 5.2.1 also writes `sec_5_2_1_hhh_optimizations.txt` beside its collected
results and copies it to the plots folder. This short report describes the
high-anchor technique and gives measured aggregate total-time speedup,
maximum-update-time speedup, and memory reduction. Ratios use repetition medians
and geometric means across valid graph pairs, without estimate contingencies.
There is no paper comparison or per-graph breakdown. The report is
generated even with `--skip-plots` and regenerated when resuming completed runs.

Docker writes directly to bind-mounted host directories: per-run files appear
as runs finish, the combined CSV is saved after each experiment, and its PDFs
are published immediately after plotting. No final Docker copy/export is
needed, and an error in a later experiment does not remove earlier outputs.
Outputs are in `exp/results/` and `exp/plots/`. Custom `--results-dir`, `--plots-dir`,
and `--data-dir` options also refer to host paths and are mounted automatically.

The collector also recovers older native CSVs whose seven final count values
were emitted without header names, leaving the raw files unchanged. Resume
without `--force` to collect those completed runs without repeating them.

```text
exp/results/<experiment>/
  manifest.json      exact commands and inputs
  runs/ssf_*.txt     one short-stat result per run
  runs/*.done        successful-run resume markers
  results.csv        collected results with exactly one CSV header
  results_csm.csv    Figure 5.5 CSM reference results, when applicable
  results_rapidflow.csv  Figure 5.5 RapidFlow results, when applicable
  results_dna.csv    Figure 5.5 freshly measured DNA aggregate results
  details/*.txt      Figure 5.4 per-update data for the fr_wiki inset only
  logs/*.log         stdout/stderr per run
exp/plots/
  sec_5_2_1_hhh_optimizations.pdf
  sec_5_2_1_hhh_optimizations.txt
  fig_5_1_hhh_epsilon.pdf
  fig_5_2_hhh_partitions.pdf
  fig_5_3_egst_partitions.pdf
  fig_5_4_static_baselines.pdf
  fig_5_5_dynamic_baselines.pdf
  fig_5_6_full_set.pdf
  fig_a_1_extra_aux.pdf
  fig_a_2_rebalance_factor.pdf
  fig_a_3_egst_optimizations.pdf
  fig_a_4_hhh_single_patterns_<pattern>.pdf
  fig_a_5_egst_single_patterns_<pattern>.pdf
```

All final plots are collected in `exp/plots/`. Raw measurements are never
deleted automatically. The plotting sources and their original style
configuration are bundled in `exp/plotting/`. Use `--force` to repeat completed
runs; remove a particular `ssf_*.txt` file or its `.done` marker to rerun only
that job.

The plotting style uses Matplotlib's bundled Computer Modern sans-serif and
math fonts instead of invoking a full LaTeX installation. Fonts are embedded
as TrueType vector text (`pdf.fonttype = 42`), keeping the paper's font style
and selectable text while substantially reducing image build time.

## Reproducibility notes

The paper used Ubuntu 24.04, GCC optimization, three repetitions, one core per
non-overlapping cache, and medians/geometric means.  The image mirrors that
software base, builds AlgoraCore and AlgoraDyn from their public default
branches, and builds this repository in release mode. For archival reproduction, pin
the dependency revisions in `Dockerfile` to the commits used for the final
paper release.

Full-set runs can take days and individual jobs have a 24-hour timeout. Static
baselines use 150K updates. The HHH/EGST and RapidFlow dynamic-baseline commands
use a five-hour limit; ContinuousSubgraphMatching passes no explicit time-limit,
matching the supplied paper script. Avoid running unrelated workloads on the
selected cache groups.
