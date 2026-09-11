#!/usr/bin/env python3
"""Copy a stream prefix while densely renumbering its vertices."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("limit", type=int)
    args = parser.parse_args()

    vertices: dict[int, int] = {}
    written = 0
    temporary = args.output.with_suffix(args.output.suffix + ".part")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.input.open() as source, temporary.open("w") as target:
        for line in source:
            fields = line.split()
            if len(fields) < 3:
                continue
            u, v = int(fields[0]), int(fields[1])
            dense_u = vertices.setdefault(u, len(vertices))
            dense_v = vertices.setdefault(v, len(vertices))
            target.write(" ".join((str(dense_u), str(dense_v), *fields[2:])) + "\n")
            written += 1
            if written == args.limit:
                break
    if written != args.limit:
        temporary.unlink(missing_ok=True)
        raise SystemExit(
            f"{args.input} contains only {written} valid updates; {args.limit} required"
        )
    temporary.replace(args.output)


if __name__ == "__main__":
    main()
