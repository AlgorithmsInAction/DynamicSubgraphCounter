#!/usr/bin/env bash
set -Eeuo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
DATA_DIR="$ROOT/exp/data"
DRY_RUN=0
TOY=0
SUITES=()

while (($#)); do
    case "$1" in
        --data-dir) DATA_DIR=$2; shift 2 ;;
        --suite) SUITES+=("$2"); shift 2 ;;
        --dry-run) DRY_RUN=1; shift ;;
        --toy) TOY=1; shift ;;
        *) echo "get_data.sh: unknown option: $1" >&2; exit 2 ;;
    esac
done
((${#SUITES[@]})) || SUITES=(engineering_set_main engineering_set_appendix full_set)

FULL="$DATA_DIR/full"
SMALL="$DATA_DIR/2m"
TINY="$DATA_DIR/150k"
CSM="$DATA_DIR/2m_csm_format"
TOY_DIR="$DATA_DIR/2k"
TOY_CSM="$DATA_DIR/2k_csm_format"
RAW="$DATA_DIR/raw"
mkdir -p "$FULL" "$SMALL" "$TINY" "$CSM" "$TOY_DIR" "$TOY_CSM" "$RAW"

ES_GRAPHS=(
  simple_wiki nl_wiki pl_wiki it_wiki fr_wiki de_wiki
  ia-enron-email-dynamic_f sx-askubuntu_f sx-superuser_f wiki-talk-temporal_f
  soc-youtube-growth_f sx-stackoverflow_f
  c_answers c_delicious c_ca-dblp c_web-notredame c_as-routeviews c_atp-gr-qc
  g_epinions g_flickr g_blog-nat06all g_cit-hep-ph g_as-newman g_bio-proteins
)
KRON=(answers as-newman as-routeviews atp-gr-qc bio-proteins blog-nat05-6m
  blog-nat06all ca-dblp ca-gr-qc ca-hep-ph ca-hep-th cit-hep-ph cit-hep-th
  delicious epinions flickr gnutella-25 gnutella-30 web-notredame)
WIKI=(simple nl pl it fr de)
SNAP=(sx-superuser sx-askubuntu wiki-talk-temporal sx-stackoverflow sx-mathoverflow)
SNAP_TSV=(soc-redditHyperlinks-body soc-redditHyperlinks-title)
NETWORK=(ia-enron-email-dynamic ia-reality-call soc-wiki-elec tech-as-topology soc-youtube-growth)

want_full=0
want_engineering=0
want_150k=0
want_csm=0
for suite in "${SUITES[@]}"; do
    [[ $suite == full_set ]] && want_full=1
    [[ $suite == engineering_set_main || $suite == engineering_set_appendix ]] && want_engineering=1
    [[ $suite == engineering_set_main ]] && { want_150k=1; want_csm=1; }
done
if ((TOY)); then
    [[ ${#SUITES[@]} == 1 && ${SUITES[0]} == engineering_set_main ]] || {
        echo "--toy requires --suite engineering_set_main" >&2
        exit 2
    }
    want_150k=0
fi

have_all() {
    local dir=$1; shift
    local graph
    for graph; do [[ -s "$dir/$graph.e" ]] || return 1; done
}

if ((DRY_RUN)); then
    echo "Data directory: $DATA_DIR"
    echo "Sources: dyreach.taa.univie.ac.at (Kron/Wiki), SNAP, Network Repository"
    echo "Would prepare: $([[ $TOY == 1 ]] && echo 'toy ES (24 graphs, 2,000 updates each)' || { [[ $want_full == 1 ]] && echo 'FS (56 graphs)' || true; [[ $want_full == 1 && $want_engineering == 1 ]] && echo ' + ' || true; [[ $want_engineering == 1 ]] && echo 'ES (24 graphs)' || true; })"
    exit 0
fi
command -v graph_cleaner >/dev/null || { echo "graph_cleaner is not on PATH (use Docker or compile exp/tools/graph_cleaner.cpp)" >&2; exit 1; }
command -v temporal_to_updates >/dev/null || { echo "temporal_to_updates is not on PATH (use Docker or compile exp/tools/unixTo24hLifetime.cpp)" >&2; exit 1; }

download() {
    local url=$1 output=$2
    [[ -s $output ]] && return 0
    echo "[data] download $(basename "$output")"
    if ! wget --continue --quiet --no-check-certificate "$url" -O "$output.part"; then
        echo "[data] download failed: $url" >&2
        return 1
    fi
    mv "$output.part" "$output"
}

clean_dynamic() {
    local input=$1 output=$2 temp
    [[ -s $output ]] && return 0
    temp="$RAW/clean-$(basename "$output" .e)"
    cp --reflink=auto "$input" "$temp"
    graph_cleaner "$temp" >/dev/null
    mv "$temp.e" "$output"
    rm -f "$temp"
}

prepare_temporal() {
    local input=$1 output=$2 stem generated
    [[ -s $output ]] && return 0
    stem=${input%.*}
    temporal_to_updates "$input" >/dev/null
    generated="${stem}_f"
    clean_dynamic "$generated" "$output"
    rm -f "${stem}_d" "${stem}_w" "$generated"
}

prepare_kron() {
    local prefix name archive unpacked output
    prefix=$1
    name=$2
    archive="$RAW/${prefix}_${name}.xz"
    output="$FULL/${prefix}_${name}.e"
    [[ -s $output ]] && return 0
    download "https://dyreach.taa.univie.ac.at/assets/data/kronecker-$([[ $prefix == c ]] && echo csize || echo growing)/${name}.xz" "$archive"
    unpacked=${archive%.xz}
    if [[ ! -s $unpacked ]]; then xz --decompress --stdout "$archive" > "$unpacked.part"; mv "$unpacked.part" "$unpacked"; fi
    clean_dynamic "$unpacked" "$output"
}

prepare_wiki() {
    local country archive unpacked output
    country=$1
    archive="$RAW/${country}_wiki.xz"
    output="$FULL/${country}_wiki.e"
    [[ -s $output ]] && return 0
    download "https://dyreach.taa.univie.ac.at/assets/data/konect/link-dynamic-${country}wiki.xz" "$archive"
    unpacked=${archive%.xz}
    if [[ ! -s $unpacked ]]; then xz --decompress --stdout "$archive" > "$unpacked.part"; mv "$unpacked.part" "$unpacked"; fi
    clean_dynamic "$unpacked" "$output"
}

prepare_snap() {
    local name ext archive raw output
    name=$1
    ext=$2
    archive="$RAW/${name}.${ext}.gz"
    raw="$RAW/${name}.${ext}"
    output="$FULL/${name}_f.e"
    [[ -s $output ]] && return 0
    download "https://snap.stanford.edu/data/${name}.${ext}.gz" "$archive"
    if [[ ! -s $raw ]]; then gzip --decompress --stdout "$archive" > "$raw.part"; mv "$raw.part" "$raw"; fi
    prepare_temporal "$raw" "$output"
}

prepare_snap_tsv() {
    local name raw output
    name=$1
    raw="$RAW/${name}.tsv"
    output="$FULL/${name}_f.e"
    [[ -s $output ]] && return 0
    download "https://snap.stanford.edu/data/${name}.tsv" "$raw"
    prepare_temporal "$raw" "$output"
}

prepare_network() {
    local name archive dir source output
    name=$1
    archive="$RAW/${name}.zip"
    dir="$RAW/unzip-$name"
    output="$FULL/${name}_f.e"
    [[ -s $output ]] && return 0
    download "https://nrvis.com/download/data/dynamic/${name}.zip" "$archive"
    mkdir -p "$dir"
    unzip -oq "$archive" -d "$dir"
    source=$(find "$dir" -type f \( -name '*.edges' -o -name '*.txt' -o -name '*.csv' -o -name '*.tsv' \) ! -iname 'readme*' -print -quit)
    [[ -n $source ]] || { echo "No temporal edge file in $archive" >&2; exit 1; }
    prepare_temporal "$source" "$output"
}

if ((want_full)); then
    for country in "${WIKI[@]}"; do prepare_wiki "$country"; done
    for name in "${KRON[@]}"; do prepare_kron c "$name"; prepare_kron g "$name"; done
    for name in "${SNAP[@]}"; do prepare_snap "$name" txt; done
    for name in "${SNAP_TSV[@]}"; do prepare_snap_tsv "$name"; done
    for name in "${NETWORK[@]}"; do prepare_network "$name"; done
else
    for country in "${WIKI[@]}"; do prepare_wiki "$country"; done
    for spec in c_answers c_delicious c_ca-dblp c_web-notredame c_as-routeviews c_atp-gr-qc \
                g_epinions g_flickr g_blog-nat06all g_cit-hep-ph g_as-newman g_bio-proteins; do
        prepare_kron "${spec%%_*}" "${spec#*_}"
    done
    for name in sx-askubuntu sx-superuser wiki-talk-temporal sx-stackoverflow; do prepare_snap "$name" txt; done
    for name in ia-enron-email-dynamic soc-youtube-growth; do prepare_network "$name"; done
fi

if ((want_engineering)); then
    if ((TOY)); then
        echo "[data] deriving 2K toy streams"
    else
        echo "[data] deriving 2M streams"
    fi
    for graph in "${ES_GRAPHS[@]}"; do
        source="$FULL/$graph.e"
        [[ -s $source ]] || { echo "Missing prepared ES graph: $source" >&2; exit 1; }
        if ((TOY)); then
            if [[ ! -s "$TOY_DIR/${graph}_2k.e" ]]; then
                python3 "$ROOT/exp/tools/create_toy_stream.py" "$source" "$TOY_DIR/${graph}_2k.e" 2000
            fi
        else
            if [[ ! -s "$SMALL/${graph}_2m.e" ]]; then head -n 2000000 "$source" > "$SMALL/${graph}_2m.e.part"; mv "$SMALL/${graph}_2m.e.part" "$SMALL/${graph}_2m.e"; fi
            if ((want_150k)) && [[ ! -s "$TINY/${graph}_150k.e" ]]; then head -n 150000 "$source" > "$TINY/${graph}_150k.e.part"; mv "$TINY/${graph}_150k.e.part" "$TINY/${graph}_150k.e"; fi
        fi
    done
fi

missing_csm=0
if ((want_csm)); then
  for graph in "${ES_GRAPHS[@]}"; do
      if ((TOY)); then
          [[ -s "$TOY_CSM/${graph}_2k.updates" && -s "$TOY_CSM/${graph}_2k.vertices" ]] || missing_csm=1
      else
          [[ -s "$CSM/${graph}_2m.updates" && -s "$CSM/${graph}_2m.vertices" ]] || missing_csm=1
      fi
  done
fi
if ((want_csm && missing_csm)); then
    csm_stage=$(mktemp -d "$DATA_DIR/.csm-stage.XXXXXX")
    if ((TOY)); then
        python3 "$ROOT/exp/tools/create_csm_format.py" "$TOY_DIR" "$csm_stage" >/dev/null
        for generated in "$csm_stage"/*; do mv "$generated" "$TOY_CSM/"; done
    else
        python3 "$ROOT/exp/tools/create_csm_format.py" "$SMALL" "$csm_stage" >/dev/null
        for generated in "$csm_stage"/*; do mv "$generated" "$CSM/"; done
    fi
    rmdir "$csm_stage"
fi
if ((TOY)); then
    printf '[data] ready: %s full, %s toy 2K streams\n' \
        "$(find "$FULL" -name '*.e' | wc -l)" "$(find "$TOY_DIR" -name '*.e' | wc -l)"
else
    printf '[data] ready: %s full, %s 2M, %s 150K streams\n' \
        "$(find "$FULL" -name '*.e' | wc -l)" "$(find "$SMALL" -name '*.e' | wc -l)" "$(find "$TINY" -name '*.e' | wc -l)"
fi
