#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import itertools
import json
import math
import os
import shutil
import subprocess
import sys
import re
import statistics
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
from dataclasses import dataclass
from pathlib import Path

ES = [
    "simple_wiki", "nl_wiki", "pl_wiki", "it_wiki", "fr_wiki", "de_wiki",
    "ia-enron-email-dynamic_f", "sx-askubuntu_f", "sx-superuser_f", "wiki-talk-temporal_f",
    "soc-youtube-growth_f", "sx-stackoverflow_f", "c_answers", "c_delicious", "c_ca-dblp",
    "c_web-notredame", "c_as-routeviews", "c_atp-gr-qc", "g_epinions", "g_flickr",
    "g_blog-nat06all", "g_cit-hep-ph", "g_as-newman", "g_bio-proteins",
]
FS = [
    "simple_wiki", "nl_wiki", "pl_wiki", "it_wiki", "fr_wiki", "de_wiki",
    "ia-reality-call_f", "soc-wiki-elec_f", "tech-as-topology_f", "soc-redditHyperlinks-body_f",
    "sx-mathoverflow_f", "ia-enron-email-dynamic_f", "soc-redditHyperlinks-title_f",
    "sx-askubuntu_f", "sx-superuser_f", "wiki-talk-temporal_f", "soc-youtube-growth_f",
    "sx-stackoverflow_f",
] + [f"c_{x}" for x in (
    "answers delicious epinions flickr blog-nat05-6m blog-nat06all cit-hep-ph cit-hep-th "
    "ca-dblp ca-gr-qc ca-hep-th ca-hep-ph web-notredame as-newman as-routeviews "
    "gnutella-30 gnutella-25 atp-gr-qc bio-proteins").split()] + [f"g_{x}" for x in (
    "answers delicious epinions flickr blog-nat05-6m blog-nat06all cit-hep-ph cit-hep-th "
    "ca-dblp ca-gr-qc ca-hep-ph ca-hep-th web-notredame as-newman as-routeviews "
    "gnutella-25 gnutella-30 atp-gr-qc bio-proteins").split()]
PATTERNS = {"all": "--all", "triangle": "--triangle", "tPath": "--tPath", "paw": "--paw",
            "fCycle": "--fCycle", "diamond": "--diamond", "fClique": "--fClique"}
EXP_ROOT = Path(__file__).resolve().parents[1]
MEMORY_RESERVATIONS = EXP_ROOT / "resources/memory_reservations.csv"
TOY_RESERVATION_KIB = 1024 * 1024
TOY_EXPERIMENTS = {
    "sec_5_2_1_hhh_optimizations", "fig_5_1_hhh_epsilon",
    "fig_5_2_hhh_partitions", "fig_5_3_egst_partitions",
    "fig_5_4_static_baselines", "fig_5_5_dynamic_baselines",
}


@dataclass(frozen=True)
class Config:
    label: str
    args: tuple[str, ...]


@dataclass(frozen=True)
class Job:
    graph: str
    pattern: str
    config: Config
    attempt: int
    data_set: str = "2m"
    timeout: int = 1
    reference: str = ""

    @property
    def key(self) -> str:
        safe = self.config.label.replace("/", "_")
        return f"{self.graph}__{self.pattern}__{safe}__run{self.attempt}"


def frange(start: int, stop: int, denominator: int) -> list[str]:
    return [f"{i / denominator:.3f}" for i in range(start, stop + 1)]


def eps_config(algo: str, mode: str, epsilon: str, extra: tuple[str, ...] = ()) -> Config:
    mode_args = () if mode == "orig" else ("-m", mode)
    return Config(f"{algo}_{mode}_e{epsilon}", ("-a", algo, "-p", "epstab", "-e", epsilon) + mode_args + extra)


def h_config(algo: str, factor: str, extra: tuple[str, ...] = ()) -> Config:
    return Config(f"{algo}_h{factor}", ("-a", algo, "-p", "hindex", "--gradual_factor", factor) + extra)


HHH_best = ("--highAnchorsOnly", "--extraAux")
EGST_best = ("--highAnchorsOnly", "--direct")


def jobs_for(name: str) -> list[Job]:
    jobs: list[Job] = []
    def add(graphs, patterns, configs, data_set="2m", timeout=1):
        jobs.extend(Job(g, p, c, r, data_set, timeout)
                    for r, p, c, g in itertools.product(range(1, 4), patterns, configs, graphs))

    if name == "sec_5_2_1_hhh_optimizations":
        add(ES, ["all"], [eps_config("hhh", "orig", "0.417"),
                           eps_config("hhh", "orig", "0.417", ("--highAnchorsOnly",))])
        # Labels must distinguish base HHH* from the high-anchor variant.
        jobs = [Job(j.graph, j.pattern,
                    Config("hhh_H_orig_e0.417" if "--highAnchorsOnly" in j.config.args else "hhh_base_orig_e0.417", j.config.args),
                    j.attempt, j.data_set, j.timeout) for j in jobs]
    elif name == "fig_5_1_hhh_epsilon":
        add(ES, ["all"], [eps_config("hhh", "orig", e, HHH_best + ("--count_stats", "--partition-stats"))
                           for e in frange(3, 12, 30)])
    elif name == "fig_5_2_hhh_partitions":
        configs = [eps_config("hhh", mode, e, HHH_best) for mode in ("orig", "soft", "lazy", "late")
                   for e in frange(3, 12, 30)]
        configs += [h_config("hhh", f"{1 + 1 / (2 ** x):.6f}", HHH_best) for x in range(11)]
        add(ES, ["all"], configs)
    elif name == "fig_5_3_egst_partitions":
        configs = [eps_config("egst", mode, e, EGST_best) for mode in ("lazy", "late")
                   for e in frange(9, 14, 30)]
        configs += [h_config("egst", f"{1 + 1 / (2 ** x):.6f}", EGST_best) for x in range(11)]
        add(ES, ["all"], configs)
    elif name == "fig_5_4_static_baselines":
        configs = [Config("ob", ("-a", "ob")), Config("escape", ("-a", "escape")),
                   eps_config("hhh", "orig", "0.417"), h_config("egst", "2"),
                   eps_config("hhh", "late", "0.2", HHH_best), h_config("egst", "1.0625", EGST_best)]
        add(ES, ["all"], configs, "150k", 24)
    elif name == "fig_5_5_dynamic_baselines":
        patterns = ["triangle", "tPath", "paw", "fCycle", "diamond", "fClique", "all"]
        for pattern in patterns:
            if pattern == "all":
                hc, ec = eps_config("hhh", "late", "0.2", HHH_best), h_config("egst", "1.0625", EGST_best)
            elif pattern == "triangle":
                hc = Config("hhh_scan", ("-a", "hhh", "-p", "mock",
                                         "--highAnchorsOnly", "--no_aux_for_t"))
                ec = Config("egst_scan", ("-a", "egst", "-p", "mock", "--highAnchorsOnly", "--direct"))
            elif pattern == "fClique":
                hc = Config("hhh_scan", ("-a", "hhh", "-p", "mock", "--highAnchorsOnly"))
                ec = Config("egst_scan", ("-a", "egst", "-p", "mock", "--highAnchorsOnly", "--direct"))
            elif pattern == "tPath":
                hc, ec = eps_config("hhh", "late", "0.333", ("--highAnchorsOnly",)), eps_config("egst", "late", "0.333", EGST_best)
            elif pattern == "diamond":
                hc, ec = eps_config("hhh", "late", "0.233", ("--highAnchorsOnly",)), h_config("egst", "1.0625", EGST_best)
            else:
                hc, ec = eps_config("hhh", "late", "0.233", ("--highAnchorsOnly",)), eps_config("egst", "late", "0.333" if pattern == "fCycle" else "0.433", EGST_best)
            add(ES, [pattern], [hc, ec], timeout=5)
        # Match the supplied reference grid: Graphflow and SymBi support all
        # six queries; IEDyn and TurboFlux are evaluated on the path query;
        # RapidFlow is evaluated on all six queries.
        ref_patterns = ("triangle", "tPath", "paw", "fCycle", "diamond", "fClique")
        for attempt, graph, pattern, algo in itertools.product(
                range(1, 4), ES, ref_patterns, ("graphflow", "symbi", "rapidflow")):
            jobs.append(Job(graph, pattern, Config(algo, ()), attempt,
                            "csm", 5, algo))
        for attempt, graph, algo in itertools.product(
                range(1, 4), ES, ("iedyn", "turboflux")):
            jobs.append(Job(graph, "tPath", Config(algo, ()), attempt,
                            "csm", 5, algo))
        for attempt, graph, pattern in itertools.product(range(1, 4), ES, ("triangle", "all")):
            jobs.append(Job(graph, pattern, Config("dna", ()), attempt, "2m", 5, "dna"))
    elif name == "fig_5_6_full_set":
        add(FS, ["all"], [eps_config("hhh", "orig", "0.417", HHH_best), eps_config("hhh", "late", "0.2", HHH_best),
                           h_config("egst", "2", EGST_best), h_config("egst", "1.0625", EGST_best)], "full", 24)
    elif name == "fig_a_1_extra_aux":
        required = Config("hhh_required_aux", ("-a", "hhh", "-p", "epstab", "-e", "0.417", "--highAnchorsOnly"))
        all_aux = Config("hhh_all_aux", ("-a", "hhh", "-p", "epstab", "-e", "0.417", "--highAnchorsOnly", "--extraAux"))
        no_aux = Config("hhh_no_aux", ("-a", "hhh", "-p", "mock", "--highAnchorsOnly", "--no_aux_for_t"))
        # Figure A.1 contains 3 triangle, 2 clique, and 2 all-pattern variants.
        add(ES, ["triangle"], [no_aux, required, all_aux])
        add(ES, ["fClique"], [no_aux, all_aux])
        add(ES, ["all"], [required, all_aux])
    elif name == "fig_a_2_rebalance_factor":
        configs = []
        for mode in ("orig", "soft", "lazy", "late"):
            factors = ("1", "1.25", "1.5", "2", "3", "5", "8", "13", "21") if mode == "lazy" else ("1.5", "2", "3", "5", "8", "13", "21")
            for factor in factors:
                base = eps_config("hhh", mode, "0.2", HHH_best)
                configs.append(Config(f"{base.label}_rho{factor}", base.args + ("-r", factor)))
        add(ES, ["all"], configs)
    elif name == "fig_a_3_egst_optimizations":
        configs = [h_config("egst", "2", flags) for flags in ((), ("--direct",), EGST_best, ("--highAnchorsOnly",))]
        configs = [Config(f"egst_{i}", c.args) for i, c in zip(("base", "D", "HD", "H"), configs)]
        add(ES, ["all"], configs)
    elif name in ("fig_a_4_hhh_single_patterns", "fig_a_5_egst_single_patterns"):
        algo = "hhh" if "hhh" in name else "egst"
        extra = HHH_best if algo == "hhh" else EGST_best
        modes = ("orig", "soft", "lazy", "late") if algo == "hhh" else ("lazy", "late")
        hconfigs = [h_config(algo, f"{1 + 1 / (2 ** x):.6f}", extra) for x in range(11)]
        for pattern in ("triangle", "tPath", "paw", "fCycle", "diamond", "fClique"):
            if algo == "hhh" and pattern in ("triangle", "tPath"):
                epsilon_values = frange(6, 25, 30)
            else:
                epsilon_values = frange(9, 14, 30) if algo == "egst" else frange(3, 12, 30)
            configs = [eps_config(algo, mode, e, extra) for mode in modes for e in epsilon_values] + hconfigs
            add(ES, [pattern], configs)
    else:
        raise ValueError(f"unknown experiment {name}")
    return jobs


def parse_cpu_set(value: str) -> list[int]:
    result: set[int] = set()
    for part in value.split(","):
        bounds = part.strip().split("-", 1)
        if not bounds[0]:
            raise ValueError("empty CPU item")
        if len(bounds) == 1:
            result.add(int(bounds[0]))
        else:
            result.update(range(int(bounds[0]), int(bounds[1]) + 1))
    allowed = set(os.sched_getaffinity(0))
    invalid = result - allowed
    if invalid:
        raise ValueError(f"CPUs not available to this process: {sorted(invalid)}; allowed: {sorted(allowed)}")
    return sorted(result)


def numa_node_for_cpu(cpu: int) -> int:
    """Return the NUMA node that owns a logical CPU according to Linux sysfs."""
    cpu_dir = Path(f"/sys/devices/system/cpu/cpu{cpu}")
    nodes = []
    for path in cpu_dir.glob("node[0-9]*"):
        try:
            nodes.append(int(path.name.removeprefix("node")))
        except ValueError:
            continue
    if len(nodes) != 1:
        raise ValueError(
            f"cannot determine a unique NUMA node for CPU {cpu} from {cpu_dir}; "
            f"found {sorted(nodes)}"
        )
    return nodes[0]


def memory_requirements_kib(experiment: str, jobs: list[Job]) -> dict[str, int]:
    """Load per-configuration estimates based on previous peak-RSS measurements."""
    if not MEMORY_RESERVATIONS.is_file():
        raise SystemExit(f"memory reservation metadata not found: {MEMORY_RESERVATIONS}")
    reservations: dict[str, int] = {}
    with MEMORY_RESERVATIONS.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if row.get("experiment") == experiment:
                reservations[row["job"]] = int(row["memory_kib"])
    requirements = {}
    missing = []
    for job in jobs:
        if job.reference:
            continue
        calibration_key = job.key.rsplit("__run", 1)[0]
        value = reservations.get(calibration_key)
        if value is None:
            missing.append(calibration_key)
        else:
            requirements[job.key] = value
    if missing:
        raise SystemExit(
            f"missing memory reservations for {experiment}: "
            + ", ".join(sorted(set(missing))[:20])
        )
    return requirements


def parse_reference_log(job: Job, log_path: Path, result: Path, returncode: int) -> None:
    """Normalize upstream stdout into the same seconds/KiB schema as our runs."""
    text = log_path.read_text(errors="replace")
    def number(pattern: str) -> float | None:
        match = re.search(pattern, text, re.MULTILINE)
        return float(match.group(1)) if match else None

    if job.reference == "rapidflow":
        seconds = number(r"^Query time \(seconds\):\s*([0-9.eE+-]+)")
        steps = number(r"^Edge process count:\s*(\d+)")
        status = "timeout" if "Time out..." in text else ("ok" if returncode == 0 else "failed")
    else:
        milliseconds = number(r"^Incremental Matching:\s*([0-9.eE+-]+)ms")
        seconds = milliseconds / 1000.0 if milliseconds is not None else None
        steps = number(r"^(\d+) edge updates\.")
        status = "ok" if returncode == 0 else ("timeout" if seconds is not None else "failed")
    result.parent.mkdir(parents=True, exist_ok=True)
    with result.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=(
            "graph", "pattern", "algo", "attempt", "steps", "time", "status", "returncode"))
        writer.writeheader()
        writer.writerow({"graph": job.graph, "pattern": job.pattern, "algo": job.reference,
                         "attempt": job.attempt, "steps": "" if steps is None else int(steps),
                         "time": "" if seconds is None else seconds,
                         "status": status, "returncode": returncode})


FINAL_COUNT_COLUMNS = ["tCycle_final", "diamonds_final", "tPath_final",
                       "fCycles_final", "claws_final", "fCliques_final", "paws_final"]


def collect_results(jobs: list[Job], runs: Path, target: Path) -> int:
    """Merge heterogeneous ssf schemas by column name, with blank missing values."""
    sources: list[tuple[Path, list[dict[str, str]]]] = []
    fieldnames: list[str] = []
    seen: set[str] = set()
    for job in jobs:
        path = runs / f"ssf_{job.key}.txt"
        if not path.is_file() or path.stat().st_size == 0:
            continue
        with path.open(newline="") as stream:
            reader = csv.reader(stream)
            header = next(reader, [])
            if not header:
                continue
            values_rows = [values for values in reader if values]
            # Older native binaries always emitted seven final counts, but
            # omitted their names unless --count_stats was enabled. Recover
            # only that known schema defect, keeping memory as the last field.
            if (not job.reference and header[-1] == "memory"
                    and not any(name.endswith("_final") for name in header)
                    and values_rows
                    and all(len(values) == len(header) + 7 for values in values_rows)):
                header = header[:-1] + FINAL_COUNT_COLUMNS + header[-1:]
            # With --count_stats, older binaries named these seven values in
            # statistics order rather than print_current_output_global_count order.
            old_final = ["tCycle_final", "tPath_final", "claws_final", "paws_final",
                         "fCycles_final", "diamonds_final", "fCliques_final"]
            if not job.reference and header[-8:] == old_final + ["memory"]:
                header = header[:-8] + FINAL_COUNT_COLUMNS + ["memory"]
            # Partition implementations repeat unused padding columns. These
            # carry no measurements and must not become ambiguous CSV keys.
            columns = [(index, name) for index, name in enumerate(header)
                       if name and name != "noentry"]
            names = [name for _, name in columns]
            if len(names) != len(set(names)):
                raise SystemExit(f"duplicate measurement columns in {path}")
            for name in names:
                if name not in seen:
                    seen.add(name)
                    fieldnames.append(name)
            rows = []
            for line_number, values in enumerate(values_rows, 2):
                if len(values) != len(header):
                    raise SystemExit(f"malformed ssf row in {path}:{line_number}: "
                                     f"expected {len(header)} fields, got {len(values)}")
                rows.append({name: values[index] for index, name in columns})
        sources.append((path, rows))

    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_suffix(target.suffix + ".part")
    row_count = 0
    with temporary.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        for _, rows in sources:
            for row in rows:
                writer.writerow(row)
                row_count += 1
    temporary.replace(target)
    return row_count


def report_hhh_optimizations(results: Path, output: Path, toy: bool = False) -> None:
    """Report Section 5.2.1 ratios from native measurements, without margins."""
    metrics = [("time", "Total-time speedup"),
               ("max_update", "Maximum-update-time speedup"),
               ("memory", "Memory reduction")]
    samples = {}
    with results.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if row["pattern"] != "all" or row["algo"] not in ("hhh", "v_hhh"):
                continue
            for metric, _ in metrics:
                value = float(row[metric])
                samples.setdefault((row["graph"], row["algo"], metric), []).append(value)
    graphs = sorted({key[0] for key in samples})
    lines = ["HHH high-anchor optimization (Section 5.2.1)",
             "Dataset: " + ("toy engineering set, 2,000 updates" if toy else "engineering set"),
             "HHH*-H restricts auxiliary-count anchors to high-degree vertices. Both",
             "variants use orig.417, count all patterns, and disable extraAux.",
             "Measured ratios are HHH* / HHH*-H: medians over repetitions followed",
             "by geometric means across valid graph pairs, without estimate margins.", ""]
    for metric, label in metrics:
        ratios = []
        for graph in graphs:
            base = samples.get((graph, "v_hhh", metric), [])
            high = samples.get((graph, "hhh", metric), [])
            b = statistics.median(base) if base else float("nan")
            h = statistics.median(high) if high else float("nan")
            valid = math.isfinite(b) and math.isfinite(h) and b > 0 and h > 0
            if valid:
                ratios.append(b / h)
        measured = f"{statistics.geometric_mean(ratios):.2f}x" if ratios else "N/A"
        lines.append(f"{label}: {measured}")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(".txt.part")
    temporary.write_text("\n".join(lines) + "\n")
    temporary.replace(output)
    print(f"Wrote {output}")


def memory_info_kib(path: Path) -> dict[str, int]:
    values = {}
    with path.open() as stream:
        for line in stream:
            fields = line.split()
            if len(fields) >= 2:
                key = fields[-3].rstrip(":") if fields[0] == "Node" else fields[0].rstrip(":")
                try:
                    values[key] = int(fields[-2] if fields[-1] == "kB" else fields[-1])
                except ValueError:
                    continue
    return values


def node_memory_kib(node: int, only_node: bool) -> tuple[int, int]:
    info = memory_info_kib(Path(f"/sys/devices/system/node/node{node}/meminfo"))
    total = info.get("MemTotal", 0)
    if only_node:
        available = memory_info_kib(Path("/proc/meminfo")).get("MemAvailable", 0)
    else:
        # Linux does not export per-node MemAvailable. This closely tracks its
        # reclaimable-memory inputs while remaining conservative.
        available = (info.get("MemFree", 0) + info.get("FilePages", 0)
                     + info.get("SReclaimable", 0) - info.get("Shmem", 0))
    return total, max(0, min(total, available))


def cgroup_available_kib() -> int | None:
    """Return remaining cgroup memory, when a finite v1/v2 limit is active."""
    candidates = [
        (Path("/sys/fs/cgroup/memory.max"), Path("/sys/fs/cgroup/memory.current")),
        (Path("/sys/fs/cgroup/memory/memory.limit_in_bytes"),
         Path("/sys/fs/cgroup/memory/memory.usage_in_bytes")),
    ]
    for maximum_path, current_path in candidates:
        try:
            maximum_text = maximum_path.read_text().strip()
            if maximum_text == "max":
                return None
            maximum = int(maximum_text)
            current = int(current_path.read_text().strip())
        except (OSError, ValueError):
            continue
        # Very large v1 values conventionally mean unlimited.
        if maximum >= 1 << 60:
            return None
        return max(0, maximum - current) // 1024
    return None


def memory_budgets(cpus: list[int], cpu_nodes: dict[int, int],
                   requirements: dict[str, int],
                   reservation_source: str = "estimate based on previous results") -> tuple[dict[int, int], int]:
    """Snapshot NUMA and cgroup budgets and verify every job can fit somewhere."""
    nodes = sorted(set(cpu_nodes.values()))
    node_budgets = {}
    descriptions = []
    for node in nodes:
        total, available = node_memory_kib(node, len(nodes) == 1)
        node_budgets[node] = available
        descriptions.append(
            f"node{node}={available / 2**20:.1f}/{total / 2**20:.1f} GiB available/total"
        )
    global_available = memory_info_kib(Path("/proc/meminfo")).get("MemAvailable", 0)
    cgroup_available = cgroup_available_kib()
    if cgroup_available is not None:
        global_available = min(global_available, cgroup_available)
        descriptions.append(f"cgroup={cgroup_available / 2**20:.1f} GiB available")
    peak_kib = max(requirements.values(), default=0)
    minimum_kib = min(requirements.values(), default=0)
    if peak_kib > global_available or not any(peak_kib <= value for value in node_budgets.values()):
        raise SystemExit(
            f"insufficient NUMA-local memory: largest pending job requires "
            f"{peak_kib / 2**20:.1f} GiB ({reservation_source}); " + "; ".join(descriptions)
        )
    print(
        f"Memory guard: per-job reservations {minimum_kib / 2**20:.3f}--"
        f"{peak_kib / 2**20:.1f} GiB ({reservation_source}); "
        + "; ".join(descriptions)
    )
    return node_budgets, global_available


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("experiment")
    p.add_argument("--cpus", required=True)
    p.add_argument("--data-dir", type=Path, required=True)
    p.add_argument("--results-dir", type=Path, required=True)
    p.add_argument("--plots-dir", type=Path, required=True)
    p.add_argument("--force", action="store_true")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--skip-plots", action="store_true")
    p.add_argument("--toy", action="store_true")
    args = p.parse_args()
    if args.toy and args.experiment not in TOY_EXPERIMENTS:
        raise SystemExit("--toy is supported only by engineering-set main experiments")
    cpus = parse_cpu_set(args.cpus)
    try:
        cpu_nodes = {cpu: numa_node_for_cpu(cpu) for cpu in cpus}
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
    jobs = jobs_for(args.experiment)
    print(f"[stage 2/4] Run {args.experiment}: {len(jobs)} runs", flush=True)
    out = args.results_dir / args.experiment
    runs = out / "runs"
    logs = out / "logs"
    details = out / "details"
    runs.mkdir(parents=True, exist_ok=True)
    logs.mkdir(parents=True, exist_ok=True)
    details.mkdir(parents=True, exist_ok=True)
    binary = Path(os.environ.get("SUBGRAPH_BINARY", Path(__file__).resolve().parents[2] / "build/Release/SubgraphCounter"))
    rapidflow_binary = Path(os.environ.get("RAPIDFLOW_BINARY", "/workspace/RapidFlow/build/streaming/RapidFlow.out"))
    csm_binary = Path(os.environ.get("CSM_BINARY", "/workspace/ContinuousSubgraphMatching/build/csm"))
    needed_binaries = {binary}
    dna_root = Path(os.environ.get("DNA_ROOT", "/workspace/subgraphcounting_DNA"))
    if any(job.reference == "dna" for job in jobs):
        needed_binaries.update({dna_root / "bin/dna/tools/DynamicUndirectedSubgraphCounter.class",
                                dna_root / "lib/guava-16.0.1.jar"})
        if not args.dry_run and shutil.which("java") is None:
            raise SystemExit("Java is required to run DNA")
    if any(job.reference == "rapidflow" for job in jobs):
        needed_binaries.add(rapidflow_binary)
    if any(job.reference and job.reference not in ("rapidflow", "dna") for job in jobs):
        needed_binaries.add(csm_binary)
    missing_binaries = [str(path) for path in needed_binaries if not path.is_file()]
    if not args.dry_run and missing_binaries:
        raise SystemExit("benchmark binaries not found: " + ", ".join(missing_binaries))
    if not args.dry_run and shutil.which("numactl") is None:
        raise SystemExit("numactl is required to bind each job's CPU and NUMA-local memory")

    pending = []
    manifest = []
    detail_files: dict[str, Path] = {}
    for job in jobs:
        result = runs / f"ssf_{job.key}.txt"
        marker = runs / f"{job.key}.done"
        if job.reference == "dna":
            size = "2k" if args.toy else "2m"
            input_file = args.data_dir / size / f"{job.graph}_{size}.e"
            input_files = (input_file,)
            command = ["numactl", "--physcpubind={cpu}", "--membind={node}",
                       "java", "-XX:ActiveProcessorCount=1", "-XX:+UseSerialGC",
                       "-cp", f"{dna_root}/bin:{dna_root}/lib/guava-16.0.1.jar",
                       "dna.tools.DynamicUndirectedSubgraphCounter", "signed",
                       "--input", str(input_file), "--timing-mode", "aggregate-only",
                       "--counts-output", "/dev/null", "--comment-prefix", "#",
                       "--metric-set", "triangles" if job.pattern == "triangle" else "all",
                       "--stats-output", str(result), "--timeout-seconds", str(job.timeout * 3600)]
        elif job.reference:
            csm_size = "2k" if args.toy else "2m"
            input_file = args.data_dir / f"{csm_size}_csm_format" / f"{job.graph}_{csm_size}.vertices"
            updates = input_file.with_suffix(".updates")
            query = EXP_ROOT / "resources/csm/pattern" / f"{job.pattern}.q"
            input_files = (input_file, updates, query)
            common = ["numactl", "--physcpubind={cpu}", "--membind={node}"]
            if job.reference == "rapidflow":
                command = common + [str(rapidflow_binary), "-d", str(input_file), "-q", str(query),
                                    "-u", str(updates), "-num", "429496729", "-time_limit", "18000"]
            else:
                # The original paper script did not impose CSM's documented
                # one-hour default; preserve that behavior for long queries.
                command = common + [str(csm_binary), "-q", str(query), "-d", str(input_file),
                                    "-u", str(updates), "-a", job.reference]
        else:
            input_file = (args.data_dir / "2k" / f"{job.graph}_2k.e" if args.toy else
                          args.data_dir / job.data_set / f"{job.graph}_{job.data_set}.e"
                          if job.data_set != "full" else
                          args.data_dir / "full" / f"{job.graph}.e")
            input_files = (input_file,)
            command = ["numactl", "--physcpubind={cpu}", "--membind={node}",
                       str(binary), "-i", str(input_file), *job.config.args, PATTERNS[job.pattern]]
            # The paper's Figure 5.4 contains a per-update inset for fr_wiki.
            # Preserve detailed output only for those 18 runs; all other plots
            # consume compact --ssf summaries.
            if args.experiment == "fig_5_4_static_baselines" and job.graph == "fr_wiki":
                legacy = {
                    "ob": ("oba", "none"),
                    "escape": ("escape", "none"),
                    "hhh_orig_e0.417": ("v_hhh", "epstab_0417"),
                    "egst_h2": ("egst", "hindex_2"),
                    "hhh_late_e0.2": ("hhh", "epstab_late_02"),
                    "egst_h1.0625": ("egst_high_direct", "hindex_1716"),
                }
                legacy_algo, legacy_partition = legacy[job.config.label]
                detail = details / (
                    f"{job.graph}_150k_all_{legacy_algo}_{legacy_partition}_time{job.attempt}.txt"
                )
                detail_files[job.key] = detail
                command += ["-f", str(detail)]
            command += ["--ssf", str(result), "--timeout", str(job.timeout),
                       "--dont_write_counts"]
        manifest.append({"key": job.key, "graph": job.graph, "pattern": job.pattern,
                         "config": job.config.label, "reference": job.reference or None,
                         "attempt": job.attempt, "input": str(input_file),
                         "details": str(detail_files[job.key]) if job.key in detail_files else None,
                         "command": command})
        detail = detail_files.get(job.key)
        missing_detail = detail is not None and (not detail.is_file() or detail.stat().st_size == 0)
        # Old toy markers were empty and referred to 1K streams. Only markers
        # explicitly recording the current input may resume a 2K toy job.
        stale_toy = args.toy and (not marker.is_file() or marker.read_text().strip() != str(input_file))
        if args.force or stale_toy or not marker.is_file() or not result.is_file() or result.stat().st_size == 0 or missing_detail:
            pending.append((job, result, marker, input_files, command))
    requirements = ({job.key: TOY_RESERVATION_KIB for job in jobs if not job.reference} if args.toy else
                    memory_requirements_kib(args.experiment, jobs))
    for record in manifest:
        if record["key"] in requirements:
            record["memory_required_gib"] = round(requirements[record["key"]] / 2**20, 3)
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    node_budgets = {node: 1 << 62 for node in set(cpu_nodes.values())}
    global_budget = 1 << 62
    if pending and requirements:
        pending_requirements = {
            item[0].key: requirements[item[0].key]
            for item in pending if item[0].key in requirements
        }
        if pending_requirements:
            source = "conservative toy estimate" if args.toy else "estimate based on previous results"
            node_budgets, global_budget = memory_budgets(
                cpus, cpu_nodes, pending_requirements, source
            )
    binding = ",".join(f"{cpu}->node{cpu_nodes[cpu]}" for cpu in cpus)
    completed = len(jobs) - len(pending)
    print(f"CPU/NUMA bindings: {binding}; completed: {completed}; pending: {len(pending)}")
    if args.dry_run:
        for item in pending[:10]:
            print(" ".join(item[4]).replace("{cpu}", str(cpus[0])).replace("{node}", str(cpu_nodes[cpus[0]])))
        if len(pending) > 10:
            print(f"... {len(pending) - 10} more jobs")
        return 0

    missing = sorted({str(path) for item in pending for path in item[3] if not path.is_file()})
    if missing:
        raise SystemExit("Missing prepared inputs (run without --skip-data):\n  " + "\n  ".join(missing[:20]))

    done = completed
    failures = []
    interactive_progress = sys.stdout.isatty()
    noninteractive_step = max(1, math.ceil(len(jobs) / 20))

    def report_progress(job: Job | None = None, code: int = 0, final: bool = False) -> None:
        if (not interactive_progress and job is not None and not code and not final
                and done % noninteractive_step):
            return
        percent = 100.0 * done / len(jobs) if jobs else 100.0
        failed = f"; {len(failures)} failed" if failures else ""
        current = "" if job is None else f"; {job.key}" + ("" if code == 0 else f" FAILED({code})")
        message = f"[runs {done}/{len(jobs)}; {percent:5.1f}%{failed}]{current}"
        if interactive_progress:
            print(f"\r\033[K{message}", end="\n" if final else "", flush=True)
        else:
            print(message, flush=True)

    report_progress(final=not pending)

    def execute(item, cpu):
        job, result, marker, _, command = item
        node = cpu_nodes[cpu]
        actual = [x.replace("{cpu}", str(cpu)).replace("{node}", str(node)) for x in command]
        # SubgraphCounter opens short-stat files in append mode. A pending job
        # must start with a fresh file so retries never duplicate CSV headers.
        result.unlink(missing_ok=True)
        marker.unlink(missing_ok=True)
        if job.key in detail_files:
            detail_files[job.key].unlink(missing_ok=True)
        log_path = logs / f"{job.key}.log"
        with log_path.open("w") as log:
            proc = subprocess.run(actual, stdout=log, stderr=subprocess.STDOUT)
        if job.reference == "dna":
            # DNA writes its aggregate timing CSV itself, including timeout
            # status. A launch failure must remain retryable, not become done.
            valid = False
            if proc.returncode == 0 and result.is_file():
                with result.open(newline="") as stream:
                    records = list(csv.DictReader(stream))
                valid = (len(records) == 1 and
                         bool(records[0].get("overall_total_update_seconds")) and
                         records[0].get("metric_set") == ("triangles" if job.pattern == "triangle" else "all"))
            if valid:
                marker.write_text(str(item[3][0]) + "\n")
            else:
                proc = subprocess.CompletedProcess(proc.args, proc.returncode or 1)
        elif job.reference:
            parse_reference_log(job, log_path, result, proc.returncode)
            # A reference timeout or unsupported case is itself a completed
            # observation. Keep its status/return code and continue the grid.
            marker.write_text(str(item[3][0]) + "\n")
            proc = subprocess.CompletedProcess(proc.args, 0)
        elif proc.returncode == 0 and result.is_file() and result.stat().st_size:
            marker.write_text(str(item[3][0]) + "\n")
        elif proc.returncode == 0:
            proc = subprocess.CompletedProcess(proc.args, 1)
        return item, cpu, node, proc.returncode

    queue = list(pending)
    with ThreadPoolExecutor(max_workers=len(cpus)) as pool:
        active = {}
        free_cpus = list(cpus)
        node_reserved = {node: 0 for node in node_budgets}
        global_reserved = 0
        while queue or active:
            for cpu in list(free_cpus):
                node = cpu_nodes[cpu]
                chosen = next((index for index, item in enumerate(queue)
                               if requirements.get(item[0].key, 0) <= node_budgets[node] - node_reserved[node]
                               and requirements.get(item[0].key, 0) <= global_budget - global_reserved), None)
                if chosen is None:
                    continue
                item = queue.pop(chosen)
                reserved = requirements.get(item[0].key, 0)
                node_reserved[node] += reserved
                global_reserved += reserved
                free_cpus.remove(cpu)
                active[pool.submit(execute, item, cpu)] = (cpu, node, reserved)
            if not active and queue:
                raise SystemExit("memory scheduler cannot place a pending job in the available NUMA budgets")
            finished, _ = wait(active, return_when=FIRST_COMPLETED)
            for future in finished:
                cpu, reserved_node, reserved = active.pop(future)
                node_reserved[reserved_node] -= reserved
                global_reserved -= reserved
                free_cpus.append(cpu)
                free_cpus.sort()
                item, _, node, code = future.result()
                done += 1
                if code:
                    failures.append(item[0].key)
                report_progress(item[0], code, final=done == len(jobs))
    print("[stage 3/4] Collect result files", flush=True)
    collected = out / "results.csv"
    rows = collect_results([job for job in jobs if not job.reference], runs, collected)
    print(f"Collected {rows} rows from ssf_*.txt -> {collected}", flush=True)
    if any(job.reference for job in jobs):
        csm_results = out / "results_csm.csv"
        rapid_results = out / "results_rapidflow.csv"
        csm_rows = collect_results(
            [job for job in jobs if job.reference and job.reference not in ("rapidflow", "dna")], runs, csm_results)
        rapid_rows = collect_results(
            [job for job in jobs if job.reference == "rapidflow"], runs, rapid_results)
        print(f"Collected references: {csm_rows} CSM rows, {rapid_rows} RapidFlow rows", flush=True)
        dna_rows = collect_results([job for job in jobs if job.reference == "dna"],
                                   runs, out / "results_dna.csv")
        print(f"Collected DNA: {dna_rows} rows", flush=True)
    if failures:
        raise SystemExit(f"{len(failures)} jobs failed; see {logs}")
    if args.experiment == "sec_5_2_1_hhh_optimizations":
        report = out / f"{args.experiment}.txt"
        report_hhh_optimizations(collected, report, args.toy)
        args.plots_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(report, args.plots_dir / report.name)
    if not args.skip_plots:
        print("[stage 4/4] Construct original paper plot(s)", flush=True)
        plot_command = [sys.executable, str(Path(__file__).with_name("plot_results.py")),
                        args.experiment, "--results", str(collected),
                        "--output", str(args.plots_dir)]
        if args.toy:
            plot_command.append("--toy")
        subprocess.run(plot_command, check=True)
    else:
        print("[stage 4/4] Plot construction skipped", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
