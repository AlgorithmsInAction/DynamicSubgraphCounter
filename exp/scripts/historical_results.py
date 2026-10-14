"""Historical non-HHH measurements for the --only-hhh experiment mode."""
from __future__ import annotations

import csv
from collections import Counter
from decimal import Decimal
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "resources/historical/legacy"
PATTERNS = {
    "triangle": "tCycles", "tPath": "tPaths", "paw": "paws",
    "fCycle": "fCycles", "diamond": "diamonds", "fClique": "fCliques",
    "all": "all",
}


def _number(value: str | None) -> Decimal | None:
    return Decimal(value) if value not in (None, "") else None


def _arg(args: tuple[str, ...], name: str) -> str | None:
    return args[args.index(name) + 1] if name in args else None


def _job_key(job) -> tuple:
    args = job.config.args
    partition = _arg(args, "-p")
    return (job.graph + "_2m", PATTERNS[job.pattern], partition,
            {None: 0, "soft": 1, "lazy": 2, "late": 3}[_arg(args, "-m")],
            _number(_arg(args, "-e")), _number(_arg(args, "--gradual_factor")))


def _row_key(row: dict[str, str]) -> tuple:
    return (row["graph"], row["pattern"], row["partition"],
            int(float(row.get("recompute_mode") or 0)),
            _number(row.get("epsilon")) if row["partition"] == "epstab" else None,
            _number(row.get("gradual_factor")) if row["partition"] == "hindex" else None)


def _read(name: str) -> list[dict[str, str]]:
    path = ROOT / name
    if not path.is_file():
        raise SystemExit(f"missing historical results: {path}")
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))


def historical_rows(experiment: str, skipped_jobs: list) -> list[dict[str, str]]:
    """Select exactly the old non-HHH rows in this experiment's run grid."""
    native = [job for job in skipped_jobs if not job.reference]
    if not native:
        return []
    if experiment in ("fig_5_3_egst_partitions", "fig_a_5_egst_single_patterns"):
        expected = Counter(_job_key(job) for job in native)
        selected: list[dict[str, str]] = []
        found: Counter = Counter()
        for name in ("results_eps_egst.csv", "results_egst.csv"):
            for row in _read(name):
                key = _row_key(row)
                if found[key] < expected[key]:
                    selected.append(row)
                    found[key] += 1
        if found != expected:
            raise SystemExit(f"historical EGST grid incomplete for {experiment}: "
                             f"{sum(found.values())}/{sum(expected.values())} rows")
        return selected
    if experiment == "fig_5_4_static_baselines":
        rows = [row for row in _read("compare_results_150k.csv")
                if (row["algo"], row["partition"]) in {
                    ("oba", "none"), ("escape", "none"),
                    ("egst", "hindex2.0"), ("egst_high_direct", "hindex1.0625")}]
    elif experiment == "fig_5_5_dynamic_baselines":
        rows = [row for row in _read("results_ours.csv")
                if row["algo"] == "egst_high_direct"]
    elif experiment == "fig_5_6_full_set":
        rows = [row for row in _read("results_full_all.csv") if row["algo"] == "egst"]
    elif experiment == "fig_a_3_egst_optimizations":
        rows = _read("results.csv")
    else:
        raise SystemExit(f"no historical non-HHH mapping for {experiment}")
    if len(rows) != len(native):
        raise SystemExit(f"historical grid incomplete for {experiment}: "
                         f"{len(rows)}/{len(native)} rows")
    return rows


def merge_with_hhh(fresh_path: Path, target: Path,
                   historical: list[dict[str, str]]) -> None:
    """Replace old HHH rows with the freshly collected rows, preserving schemas."""
    with fresh_path.open(newline="") as stream:
        fresh = list(csv.DictReader(stream))
        fresh_fields = list(fresh[0]) if fresh else []
    fields = list(dict.fromkeys(fresh_fields + [key for row in historical for key in row]))
    temporary = target.with_suffix(target.suffix + ".part")
    with temporary.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(fresh)
        writer.writerows(historical)
    temporary.replace(target)
