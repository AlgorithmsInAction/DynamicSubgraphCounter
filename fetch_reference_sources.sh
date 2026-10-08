#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")" && pwd)
deps=${REFERENCE_DEPS_DIR:-"$root/deps"}
ob_repo=${OB_REPO:-https://github.com/schochastics/oaqc.git}
ob_ref=${OB_REF:-v2.0.0}
escape_repo=${ESCAPE_REPO:-https://bitbucket.org/seshadhri/escape.git}
escape_ref=${ESCAPE_REF:-7ec2f93c524a0b47cdc8c639602e90d1c1fceff3}
escape_patch="$root/src/escape/memory-leaks.patch"

mkdir -p "$deps"
if [[ ! -d "$deps/oaqc/.git" ]]; then
  git clone --depth 1 "$ob_repo" "$deps/oaqc"
fi
git -C "$deps/oaqc" fetch --depth 1 origin "$ob_ref" >/dev/null
git -C "$deps/oaqc" checkout -q --detach FETCH_HEAD

if [[ ! -d "$deps/escape/.git" ]]; then
  git clone --depth 1 "$escape_repo" "$deps/escape"
fi
# The checkout is reused across builds. Remove only our previous patch before
# switching revisions, then reapply it to the selected upstream source.
if git -C "$deps/escape" apply --reverse --check "$escape_patch" 2>/dev/null; then
  git -C "$deps/escape" apply --reverse "$escape_patch"
fi
if git -C "$deps/escape" fetch --depth 1 origin "$escape_ref" >/dev/null; then
  fetched_escape=1
else
  fetched_escape=0
  echo "Warning: could not refresh ESCAPE; using the cached checkout" >&2
fi
if [[ "$escape_ref" == master && "$fetched_escape" == 1 ]]; then
  git -C "$deps/escape" checkout -q -B master FETCH_HEAD
else
  git -C "$deps/escape" checkout -q "$escape_ref"
fi

[[ -f "$deps/escape/Graph.cpp" && \
   -f "$deps/escape/GraphIO.cpp" && \
   -f "$deps/escape/Escape/Digraph.h" ]] || {
  echo "Cannot locate ESCAPE sources at revision $escape_ref" >&2
  exit 1
}
if ! git -C "$deps/escape" apply --reverse --check "$escape_patch" 2>/dev/null; then
  git -C "$deps/escape" apply --check "$escape_patch"
  git -C "$deps/escape" apply "$escape_patch"
fi
