#include "DynamizedESCAPE.h"

#include "Escape/Conversion.h"
#include "Escape/Digraph.h"
#include "Escape/EdgeHash.h"
#include "Escape/FourVertex.h"
#include "Escape/GetAllCounts.h"
#include "Escape/GraphIO.h"
#include "Escape/Triadic.h"

#include <stdexcept>
#include <utility>

using namespace Escape;

bool DynamizedESCAPE::prepare() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before preparing.\n";
        return false;
    }
    graph->onArcRemove(&OnArcAddId.emplace_back(0), [&](Algora::Arc *arc) {
        removeArc = true;
        if (defer_updates)
            pending_updates.push_back(
                {arc->getTail()->getId(), arc->getHead()->getId(), false});
        else
            run();
        removeArc = false;
    });
    graph->onArcAdd(&OnArcRemoveId.emplace_back(0), [&](Algora::Arc *arc) {
        if (defer_updates)
            pending_updates.push_back(
                {arc->getTail()->getId(), arc->getHead()->getId(), true});
        else
            run();
    });
    return true;
}

void DynamizedESCAPE::clear() {
    tPath = claw = paw = fCycle = diamond = fClique = tCycle = 0;
    sTPath.clear();
    sClaw.clear();
    sPaw.clear();
    sFCycle.clear();
    sDiamond.clear();
    sFClique.clear();
    sTriangle.clear();
}

StaticGraphSnapshot DynamizedESCAPE::capture_snapshot() const {
    StaticGraphSnapshot snapshot;
    snapshot.num_vertices = graph->getSize();
    const int edge_count = graph->getNumArcs(1) - removeArc;
    snapshot.edges.resize(static_cast<std::size_t>(edge_count) * 2);
    int i = 0;
    graph->mapEdgesUntil(
        [&](const Algora::Arc *arc) {
            if (i >= edge_count)
                throw std::runtime_error(
                    "ESCAPE snapshot edge count mismatch");
            snapshot.edges[i] = arc->getTail()->getId();
            snapshot.edges[i + edge_count] = arc->getHead()->getId();
            ++i;
        },
        Algora::arcFalse);
    if (i != edge_count)
        throw std::runtime_error("ESCAPE snapshot edge count mismatch");
    return snapshot;
}

StaticGraphSnapshot DynamizedESCAPE::snapshot_current_graph() const {
    return capture_snapshot();
}

std::vector<StaticGraphUpdate> DynamizedESCAPE::take_pending_updates() {
    auto updates = std::move(pending_updates);
    pending_updates.clear();
    return updates;
}

StaticAlgorithmResult DynamizedESCAPE::compute_snapshot(
    const StaticGraphSnapshot &snapshot) const {
    StaticAlgorithmResult result;
    Graph graph_snapshot;
    graph_snapshot.nVertices = snapshot.num_vertices;
    graph_snapshot.nEdges = static_cast<EdgeIdx>(snapshot.num_edges()) * 2;
    graph_snapshot.srcs = new VertexIdx[graph_snapshot.nEdges];
    graph_snapshot.dsts = new VertexIdx[graph_snapshot.nEdges];

    const auto edge_count = snapshot.num_edges();
    for (unsigned int edge = 0; edge < edge_count; ++edge) {
        const auto tail = snapshot.edges[edge];
        const auto head = snapshot.edges[edge + edge_count];
        graph_snapshot.srcs[2 * edge] = tail;
        graph_snapshot.dsts[2 * edge] = head;
        graph_snapshot.srcs[2 * edge + 1] = head;
        graph_snapshot.dsts[2 * edge + 1] = tail;
    }

    CGraph cg = makeCSR(graph_snapshot);
    cg.sortById();
    auto start = std::chrono::high_resolution_clock::now();
    CGraph relabeled = cg.renameByDegreeOrder();
    relabeled.sortById();
    CDAG dag = degreeOrdered(&relabeled);
    dag.outlist.sortById();
    dag.inlist.sortById();
    double non_induced_four[11];
    getAllFour(&relabeled, &dag, non_induced_four);
    auto end = std::chrono::high_resolution_clock::now();
    result.elapsed = end - start;

    result.counts[0] = snapshot.num_vertices > 3
                           ? non_induced_four[4] /
                                 (snapshot.num_vertices - 3)
                           : 0;
    result.counts[1] = non_induced_four[9];
    result.counts[2] = non_induced_four[6];
    result.counts[3] = non_induced_four[8];
    result.counts[4] = non_induced_four[5];
    result.counts[5] = non_induced_four[10];
    result.counts[6] = non_induced_four[7];

    delCGraph(dag.outlist);
    delCGraph(dag.inlist);
    delCGraph(relabeled);
    delCGraph(cg);
    delGraph(graph_snapshot);
    return result;
}

void DynamizedESCAPE::apply_result(const StaticAlgorithmResult &result) {
    timeLastComputation = result.elapsed;
    tCycle = result.counts[0];
    diamond = result.counts[1];
    tPath = result.counts[2];
    fCycle = result.counts[3];
    claw = result.counts[4];
    fClique = result.counts[5];
    paw = result.counts[6];
}

void DynamizedESCAPE::run() {
    apply_result(compute_snapshot(capture_snapshot()));
}
