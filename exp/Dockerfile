FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
RUN set -eux; \
    for attempt in 1 2 3 4 5; do \
        rm -rf /var/lib/apt/lists/*; \
        if apt-get \
            -o Acquire::By-Hash=force \
            -o Acquire::Retries=5 \
            -o Acquire::http::No-Cache=true \
            -o Acquire::http::Pipeline-Depth=0 \
            update; then \
            break; \
        fi; \
        if [ "$attempt" = 5 ]; then exit 1; fi; \
        sleep $((attempt * 5)); \
    done; \
    apt-get -o Acquire::Retries=5 install -y --no-install-recommends \
        build-essential ca-certificates cmake git libboost-all-dev numactl \
        python3 python3-matplotlib python3-numpy python3-pandas python3-yaml \
        qt5-qmake openjdk-17-jdk-headless unzip wget xz-utils; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
RUN git clone --depth 1 https://gitlab.com/libalgora/AlgoraCore.git && \
    git clone --depth 1 https://gitlab.com/libalgora/AlgoraDyn.git && \
    cd AlgoraCore && ./easyCompile -g && \
    cd ../AlgoraDyn && ./easyCompile -g

# Pin the independently maintained Figure 5.5 reference implementations.
ARG RAPIDFLOW_REF=2ea4f4c35bcee95940a5e6c17a74a2a111afc2c1
ARG CSM_REF=6118049cca9b10046aed5ddd045e207c71dc53a9
RUN git clone https://github.com/shixuansun/RapidFlow.git && \
    git -C RapidFlow checkout "$RAPIDFLOW_REF" && \
    cmake -S RapidFlow -B RapidFlow/build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build RapidFlow/build -j"$(nproc)" && \
    git clone https://github.com/RapidsAtHKUST/ContinuousSubgraphMatching.git && \
    git -C ContinuousSubgraphMatching checkout "$CSM_REF" && \
    make -C ContinuousSubgraphMatching -j"$(nproc)"

ARG DNA_REF=170e4e0551e1ea238cb65bd7a2723f0f7a8a61cd
# JDK 17 derives javac's default source encoding from the locale. DNA contains
# UTF-8 source literals; Ubuntu's C.UTF-8 locale needs no extra locale package.
ENV LANG=C.UTF-8 LC_ALL=C.UTF-8
RUN git clone https://github.com/AlgorithmsInAction/subgraphcounting_DNA.git && \
    git -C subgraphcounting_DNA checkout "$DNA_REF" && \
    bash subgraphcounting_DNA/build/build-subgraph-counter.sh

COPY . /workspace/subgraph-counting
WORKDIR /workspace/subgraph-counting
RUN ./easyCompile -g && \
    g++ -O3 -std=c++17 exp/tools/graph_cleaner.cpp -o /usr/local/bin/graph_cleaner && \
    g++ -O3 -std=c++17 exp/tools/unixTo24hLifetime.cpp -o /usr/local/bin/temporal_to_updates

ENV SUBGRAPH_BINARY=/workspace/subgraph-counting/build/Release/SubgraphCounter \
    RAPIDFLOW_BINARY=/workspace/RapidFlow/build/streaming/RapidFlow.out \
    CSM_BINARY=/workspace/ContinuousSubgraphMatching/build/csm \
    DNA_ROOT=/workspace/subgraphcounting_DNA \
    MPLBACKEND=Agg MPLCONFIGDIR=/tmp/matplotlib PYTHONUNBUFFERED=1
ENTRYPOINT ["python3", "exp/run.py"]
