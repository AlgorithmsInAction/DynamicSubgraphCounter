#!/usr/bin/env bash
set -Eeuo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
IMAGE=${SUBGRAPH_EXP_IMAGE:-subgraph-counting-exp}

mkdir -p "$ROOT/exp/data" "$ROOT/exp/results" "$ROOT/exp/plots"

if [[ ${1:-} == --native ]]; then
    shift
    exec python3 "$ROOT/exp/run.py" "$@"
fi

# Help, listing, estimation, and dry runs only inspect local metadata and CPU
# topology; none requires building or starting the container.
for argument in "$@"; do
    if [[ $argument == -h || $argument == --help || $argument == --list || $argument == --estimate || $argument == --dry-run ]]; then
        exec python3 "$ROOT/exp/run.py" "$@"
    fi
done

# Check host resources before spending time rebuilding the image. Query graphs
# are checked only when the selected experiment actually runs a reference tool.
python3 "$ROOT/exp/run.py" "$@" --check-resources

docker build -f "$ROOT/exp/Dockerfile" -t "$IMAGE" "$ROOT"

docker_args=(--rm --init --user "$(id -u):$(id -g)" --cap-add SYS_NICE
    --volume "$ROOT/exp/data:/workspace/subgraph-counting/exp/data"
    --volume "$ROOT/exp/results:/workspace/subgraph-counting/exp/results"
    --volume "$ROOT/exp/plots:/workspace/subgraph-counting/exp/plots"
    --volume "$ROOT/exp/resources:/workspace/subgraph-counting/exp/resources:ro"
    --volume "$ROOT/exp/plotting:/workspace/subgraph-counting/exp/plotting:ro")

# Custom directory options denote host paths too. Mount them at their resolved
# absolute paths and pass those paths into the container.
run_args=()
while (($#)); do
    case "$1" in
        --data-dir|--results-dir|--plots-dir)
            option=$1
            (($# >= 2)) || { echo "$option needs a path" >&2; exit 2; }
            directory=$2
            shift 2
            ;;
        --data-dir=*|--results-dir=*|--plots-dir=*)
            option=${1%%=*}
            directory=${1#*=}
            shift
            ;;
        *) run_args+=("$1"); shift; continue ;;
    esac
    [[ -n $directory ]] || { echo "$option needs a path" >&2; exit 2; }
    mkdir -p -- "$directory"
    directory=$(cd -- "$directory" && pwd)
    docker_args+=(--volume "$directory:$directory")
    run_args+=("$option" "$directory")
done
set -- "${run_args[@]}"

echo "Results and plots are written directly to host-mounted directories after each experiment."

# Preserve a terminal inside Docker so the per-run progress display can update
# one line instead of printing a new line for every completed run.
[[ -t 1 ]] && docker_args+=(--tty)

# Make the container's affinity match an explicit harness --cpus selection.
for ((i=1; i<=$#; i++)); do
    if [[ ${!i} == --cpus ]]; then
        j=$((i + 1))
        [[ $j -le $# ]] || { echo "--cpus needs a value" >&2; exit 2; }
        docker_args+=(--cpuset-cpus "${!j}")
    elif [[ ${!i} == --cpus=* ]]; then
        value=${!i}
        docker_args+=(--cpuset-cpus "${value#*=}")
    fi
done

docker run "${docker_args[@]}" "$IMAGE" "$@"
