#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
app=${APP:-"$script_dir/../../build/Release/SubgraphCounter"}
data_dir=${1:-"${HOME}/data_dyn/2m"}
output_dir=${2:-"$script_dir/results_ob_127"}
workers=${WORKERS:-127}
worker_block_size=${WORKER_BLOCK_SIZE:-1024}

if [[ ! -x "$app" ]]; then
    echo "SubgraphCounter is not executable: $app" >&2
    exit 1
fi

if [[ ! -d "$data_dir" ]]; then
    echo "Input directory does not exist: $data_dir" >&2
    exit 1
fi

mkdir -p "$output_dir/counts" "$output_dir/count_stats"

instances=(
    g_flickr
    g_as-newman
    g_bio-proteins
    g_cit-hep-ph
    g_blog-nat06all
    g_epinions
    c_atp-gr-qc
    c_answers
    c_web-notredame
    c_delicious
    c_as-routeviews
    c_ca-dblp
    simple_wiki
    nl_wiki
    pl_wiki
    it_wiki
    fr_wiki
    de_wiki
    sx-superuser_f
    sx-askubuntu_f
    wiki-talk-temporal_f
    ia-enron-email-dynamic_f
    sx-stackoverflow_f
    soc-youtube-growth_f
)

echo "#Instances: ${#instances[@]}"
echo "#Workers per instance: $workers"
echo "#Updates per worker block: $worker_block_size"

# Run one instance after another. Parallelism is confined to the static
# updates of the current OB instance, avoiding cross-instance contention.
for index in "${!instances[@]}"; do
    instance=${instances[$index]}
    input="$data_dir/${instance}_2m.e"
    count_file="$output_dir/counts/${instance}_2m_all_oba_none.txt"
    count_stats_file="$output_dir/count_stats/ssf_${instance}_2m_all_oba_none.txt"

    if [[ ! -f "$input" ]]; then
        echo "Input instance does not exist: $input" >&2
        exit 1
    fi

    echo "$((index + 1))/${#instances[@]}: $instance"
    "$app" \
        --input "$input" \
        --algo ob \
        --partition none \
        --all \
        --worker "$workers" \
        --worker-block-size "$worker_block_size" \
        --count_stats \
        --result_file "$count_file" \
        --short_stats_file "$count_stats_file"
done
