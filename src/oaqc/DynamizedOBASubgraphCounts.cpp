#include "DynamizedOBASubgraphCounts.h"
#include "QuadCensus.h"

#include <stdexcept>

bool DynamizedOBASubgraphCounts::prepare() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before preparing.\n";
        return false;
    }

    graph->onArcRemove(&OnArcAddId.emplace_back(0), [&](Algora::Arc *) {
        removeArc = true;
        run();
        removeArc = false;
    });
    graph->onArcAdd(&OnArcRemoveId.emplace_back(0), [&](Algora::Arc *) {
        run();
    });
    return true;
}

void DynamizedOBASubgraphCounts::clear() {
    tPath = claw = paw = fCycle = diamond = fClique = tCycle = 0;
    sTPath.clear();
    sClaw.clear();
    sPaw.clear();
    sFCycle.clear();
    sDiamond.clear();
    sFClique.clear();
    sTriangle.clear();
}

StaticGraphSnapshot DynamizedOBASubgraphCounts::capture_snapshot() const {
    StaticGraphSnapshot snapshot;
    snapshot.num_vertices = graph->getSize();
    const int edge_count = graph->getNumArcs(1) - removeArc;
    snapshot.edges.resize(static_cast<std::size_t>(edge_count) * 2);
    int i = 0;
    graph->mapEdgesUntil(
        [&](const Algora::Arc *arc) {
            if (i >= edge_count)
                throw std::runtime_error("OB snapshot edge count mismatch");
            snapshot.edges[i] = arc->getTail()->getId();
            snapshot.edges[i + edge_count] = arc->getHead()->getId();
            ++i;
        },
        Algora::arcFalse);
    if (i != edge_count)
        throw std::runtime_error("OB snapshot edge count mismatch");
    return snapshot;
}

StaticAlgorithmResult DynamizedOBASubgraphCounts::compute_snapshot(
    const StaticGraphSnapshot &snapshot) const {
    StaticAlgorithmResult result;
    const auto edge_count = snapshot.num_edges();
    auto start = std::chrono::high_resolution_clock::now();
    oaqc::QuadCensus census(snapshot.num_vertices, edge_count,
                            snapshot.edges.data());
    auto end = std::chrono::high_resolution_clock::now();
    result.elapsed = end - start;

    const auto orbit_count = census.getNOrbitCount();
    const auto *orbits = census.nOrbits();
    const auto *mapping = census.getMapping();
    COUNTER_TYPE triangles = 0;
    for (unsigned int vertex = 0; vertex < snapshot.num_vertices; ++vertex) {
        const auto base = mapping[vertex] * orbit_count;
        result.counts[2] += orbits[base + 9] + orbits[base + 10];
        result.counts[4] += orbits[base + 11] + orbits[base + 12];
        result.counts[6] +=
            orbits[base + 13] + orbits[base + 14] + orbits[base + 15];
        result.counts[3] += orbits[base + 16];
        result.counts[1] += orbits[base + 17] + orbits[base + 18];
        result.counts[5] += orbits[base + 19];
        if (snapshot.num_vertices > 3)
            triangles += orbits[base + 7] / (snapshot.num_vertices - 3);
    }
    result.counts[0] = triangles / 3;
    for (std::size_t i = 1; i < result.counts.size(); ++i)
        result.counts[i] /= 4;
    return result;
}

void DynamizedOBASubgraphCounts::apply_result(
    const StaticAlgorithmResult &result) {
    timeLastComputation = result.elapsed;
    tCycle = result.counts[0];
    diamond = result.counts[1];
    tPath = result.counts[2];
    fCycle = result.counts[3];
    claw = result.counts[4];
    fClique = result.counts[5];
    paw = result.counts[6];
}

void DynamizedOBASubgraphCounts::run() {
    apply_result(compute_snapshot(capture_snapshot()));
}
