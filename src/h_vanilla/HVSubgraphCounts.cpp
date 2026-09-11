#include "h_vanilla/HVSubgraphCounts.h"
#include "HVSubgraphCounts.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "graph/vertex.h"
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <cmath>
#include <iostream>
#include <vector>

/**
 * @brief Initializes the Subgraph Counter. First checks if a graph is set and
 * determines which Auxiliary Counts need to be maintained. Sets up all
 * functions that are needed for edge removals, followed by the Auxiliary Counts
 * updates, followed by edge insertions. Deletes isolated vertices and inserts
 * them again once they get connected.
 *
 * @return true
 * @return false
 */
bool HVSubgraphCounts::prepare() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before preparing.\n";
        return 0;
    }

    if (objectives[8]) {
        throw std::logic_error("S count are not implemented for hhh vanilla");
        return 0;
    }

    // Determine which Auxiliary Counts are needed for which Subgraph Counts
    bool vLV{objectives[1]};
    bool t{objectives[3]};
    bool uLv{t || objectives[1] || objectives[3] || objectives[4] ||
             objectives[5]};
    bool uLLv{objectives[4]};
    bool cLV{objectives[3]};
    bool pLL{objectives[5]};
    bool uHv{objectives[1] || objectives[3] || objectives[4] || objectives[5]};
    bool cL{objectives[5] || objectives[3]};

    StrucCount.setGraph(graph);
    StrucCount.initVertexPartition();
    StrucCount.setObjectives(vLV, t, uLv, uLLv, cLV, pLL, uHv, cL);

    // Edge Removal
    StrucCount.vPart->onArcRemove(
        &StrucCount.OnEpsArcRemoveId.emplace_back(15), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif

            // Call functions for total-Counts
            if (objectives[7]) {
                if (objectives[0] == true)
                    tCycleArcChange(a, subtract);
                if (objectives[1] == true)
                    tPathArcChange(a, subtract);
                if (objectives[2] == true)
                    ClawArcChange(a, subtract);
                if (objectives[3] == true)
                    pawArcChange(a, subtract);
                if (objectives[4] == true)
                    fCycleArcChange(a, subtract);
                if (objectives[5] == true)
                    DiamondArcChange(a, subtract);
                if (objectives[6] == true)
                    fCliqueArcChange(a, subtract);
            }
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            delTimeGraph += end - start;
#endif
        });

    // Observers for maintaining the Auxilary Counts
    StrucCount.prepare();

    // Insertion

    StrucCount.vPart->onArcAdd(
        &StrucCount.OnEpsArcAddId.emplace_back(15), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // Call functions for total-Counts
            if (objectives[7]) {
                if (objectives[0] == true)
                    tCycleArcChange(a, add);
                if (objectives[1] == true)
                    tPathArcChange(a, add);
                if (objectives[2] == true)
                    ClawArcChange(a, add);
                if (objectives[3] == true)
                    pawArcChange(a, add);
                if (objectives[4] == true)
                    fCycleArcChange(a, add);
                if (objectives[5] == true)
                    DiamondArcChange(a, add);
                if (objectives[6] == true)
                    fCliqueArcChange(a, add);
            }
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTimeGraph += end - start;
#endif
        });

    return 1;
}

void HVSubgraphCounts::unsetGraph() {
    // StrucCount.unsetGraph();
    StrucCount.vPart->unsetGraph();
}

/**
 * @brief Changes the total claw count if the arc a gets inserted or deleted. If
 * a gets inserted, the function "operation" should increase the count and
 * decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::ClawArcChange(const Algora::Arc *a,
                                     CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    operation(claw, getNrClawsWithEdge(head, tail));
}

/**
 * @brief Changes the total Four-Clique count if the arc a gets inserted or
 * deleted. If a gets inserted, the function "operation" should increase the
 * count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::fCliqueArcChange(const Algora::Arc *a,
                                        CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    operation(fClique, getNrFCliquesWithEdge(head, tail));
}

/**
 * @brief Changes the total Diamond count if the arc a gets inserted or deleted.
 * If a gets inserted, the function "operation" should increase the count and
 * decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::DiamondArcChange(const Algora::Arc *a,
                                        CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    operation(diamond, getNrDiamondsWithEdge(head, tail));
}

/**
 * @brief Changes the total Three-Path count if the arc a gets inserted or
 * deleted. If a gets inserted, the function "operation" should increase the
 * count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::tPathArcChange(const Algora::Arc *a,
                                      CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    operation(tPath, getNrTPathsWithEdge(head, tail));
}

/**
 * @brief Changes the total Triangle count if the arc a gets inserted or
 * deleted. If a gets inserted, the function "operation" should increase the
 * count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::tCycleArcChange(const Algora::Arc *a,
                                       CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    operation(tCycle, getNrTriangleWithEdge(head, tail));
}

/**
 * @brief Changes the total Paw count if the arc a gets inserted or deleted. If
 * a gets inserted, the function "operation" should increase the count and
 * decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::pawArcChange(const Algora::Arc *a,
                                    CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    bool arc_removed = !edge_exist(head, tail);
    int tHeadTail = this->getNrTriangleWithEdge(head, tail);
    int tHead{0}, tTail{0};

    if (arc_removed) {
        tHead = getNrTriangle_pretended(head, head, tail);
        tTail = getNrTriangle_pretended(tail, head, tail);
    } else {
        tHead = getNrTriangle(head);
        tTail = getNrTriangle(tail);
    }
    operation(paw, getNrPawsWithEdge(head, tail, tHead, tTail, tHeadTail));
}

/**
 * @brief Changes the total Four-Cycle count if the arc a gets inserted or
 * deleted. If a gets inserted, the function "operation" should increase the
 * count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HVSubgraphCounts::fCycleArcChange(const Algora::Arc *a,
                                       CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    operation(fCycle, getNrfCyclesWithEdge(head, tail));
}
