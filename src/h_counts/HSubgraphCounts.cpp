#include "h_counts/HSubgraphCounts.h"
#include "HSubgraphCounts.h"
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
bool HSubgraphCounts::prepare() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before preparing.\n";
        return 0;
    }

    // Determine which Auxiliary Counts are needed for which Subgraph Counts
    bool vLV{objectives[1]};
    bool t{objectives[3]};
    bool uLv{t || (use_aux_for_t && objectives[0]) || objectives[1] ||
             objectives[3] || objectives[4] || objectives[5]};
    bool uLLv{objectives[4]};
    bool cLV{objectives[3]};
    bool pLL{objectives[5] || (use_all_aux && objectives[6])};
    bool uHv{(use_aux_for_t && use_all_aux && objectives[0]) || objectives[1] ||
             objectives[3] || objectives[4] || objectives[5]};
    bool cL{objectives[5] || objectives[3] || (use_all_aux && objectives[6])};

    StrucCount.setGraph(graph);
    StrucCount.initVertexPartition();
    StrucCount.setObjectives(vLV, t, uLv, uLLv, cLV, pLL, uHv, cL);

    // Edge Removal
    StrucCount.vPart->onArcRemove(
        &StrucCount.OnEpsArcRemoveId.emplace_back(15), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // Call functions for s-Counts
            if (objectives[8]) {
                if (objectives[0] == true)
                    sTriangleArcChange(a, reduceOrDelete);
                if (objectives[1] == true)
                    sTPathArcChange(a, reduceOrDelete);
                if (objectives[2] == true)
                    sClawArcChange(a, reduceOrDelete);
                if (objectives[3] == true)
                    sPawArcChange(a, reduceOrDelete);
                if (objectives[4] == true)
                    sFCycleArcChange(a, reduceOrDelete);
                if (objectives[5] == true)
                    sDiamondArcChange(a, reduceOrDelete);
                if (objectives[6] == true)
                    sFCliqueArcChange(a, reduceOrDelete);

                // Remove newly isolated vertices from the internal vertex List
                if (observedVertices.count(a->getHead()->getId()) != 0 &&
                    graph->getUndirectedDegree(a->getHead()) == 1) {
                    CurrentObservedVertices.erase(CAST_ILV(a->getHead()));
                }

                if (observedVertices.count(a->getTail()->getId()) != 0 &&
                    graph->getUndirectedDegree(a->getTail()) == 1) {
                    CurrentObservedVertices.erase(CAST_ILV(a->getTail()));
                }
            }

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
            if (objectives[8]) {

                // Reinsert isolated edges that got connected to the internal
                // Vertex List.
                if (observedVertices.count(a->getHead()->getId()) != 0 &&
                    graph->getUndirectedDegree(a->getHead()) == 1) {
                    CurrentObservedVertices.emplace(CAST_ILV(a->getHead()),
                                                    std::monostate{});
                }

                if (observedVertices.count(a->getTail()->getId()) != 0 &&
                    graph->getUndirectedDegree(a->getTail()) == 1) {
                    CurrentObservedVertices.emplace(CAST_ILV(a->getTail()),
                                                    std::monostate{});
                }

                // Call functions for s-Counts
                if (objectives[0] == true)
                    sTriangleArcChange(a, increaseOrCreate);
                if (objectives[1] == true)
                    sTPathArcChange(a, increaseOrCreate);
                if (objectives[2] == true)
                    sClawArcChange(a, increaseOrCreate);
                if (objectives[3] == true)
                    sPawArcChange(a, increaseOrCreate);
                if (objectives[4] == true)
                    sFCycleArcChange(a, increaseOrCreate);
                if (objectives[5] == true)
                    sDiamondArcChange(a, increaseOrCreate);
                if (objectives[6] == true)
                    sFCliqueArcChange(a, increaseOrCreate);
            }

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

void HSubgraphCounts::unsetGraph() {
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
void HSubgraphCounts::ClawArcChange(const Algora::Arc *a,
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
void HSubgraphCounts::fCliqueArcChange(const Algora::Arc *a,
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
void HSubgraphCounts::DiamondArcChange(const Algora::Arc *a,
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
void HSubgraphCounts::tPathArcChange(const Algora::Arc *a,
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
void HSubgraphCounts::tCycleArcChange(const Algora::Arc *a,
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
void HSubgraphCounts::pawArcChange(const Algora::Arc *a,
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
void HSubgraphCounts::fCycleArcChange(const Algora::Arc *a,
                                      CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    operation(fCycle, getNrfCyclesWithEdge(head, tail));
}

/**
 * @brief Changes the Triangle S-Count for all or the specified vertices if the
 * arc a gets inserted or deleted. If a gets inserted, the function "operation"
 * should increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sTriangleArcChange(const Algora::Arc *a,
                                         IncOrDecSingleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    operation(sTriangle, head->getId(), sTriangleCount(tail, head, head));
    operation(sTriangle, tail->getId(), sTriangleCount(head, tail, tail));
    if (graph->getUndirectedDegree(head) < graph->getUndirectedDegree(tail)) {
        for (auto const &a1 : head->getEdges()) {

            auto s = CAST_ILV(a1->getOther(head));

            if (s != tail && (all_s || CurrentObservedVertices.count(s)))
                operation(sTriangle, s->getId(), sTriangleCount(head, tail, s));
        }
    } else {
        for (auto const &a1 : tail->getEdges()) {

            auto s = CAST_ILV(a1->getOther(tail));

            if (s != head && (all_s || CurrentObservedVertices.count(s)))
                operation(sTriangle, s->getId(), sTriangleCount(tail, head, s));
        }
    }
}

/**
 * @brief Changes the Three-Path S-Count for all or the specified vertices if
 * the arc a gets inserted or deleted. If a gets inserted, the function
 * "operation" should increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sTPathArcChange(const Algora::Arc *a,
                                      IncOrDecSingleKey *operation) {

    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool headLow{is_low(head)};
    bool tailLow{is_low(tail)};

    already_observed.clear();

    auto inc = getNrTPathsWithEdge(head, tail);
    operation(sTPath, head->getId(), inc);
    already_observed.emplace(head->getId(), std::monostate{});
    operation(sTPath, tail->getId(), inc);
    already_observed.emplace(tail->getId(), std::monostate{});

    for (auto const &a : head->getEdges()) {
        auto x = CAST_ILV(a->getOther(head));
        if (x == tail)
            continue;

        if (already_observed.find(x->getId()) == already_observed.end() &&
            (all_s || CurrentObservedVertices.count(x))) {
            operation(sTPath, x->getId(),
                      sTPathCount(head, tail, x, !headLow, !tailLow));
            already_observed.emplace(x->getId(), std::monostate{});
        }

        for (auto const &a2 : x->getEdges()) {
            auto y = CAST_ILV(a2->getOther(x));

            if (y != head && y != tail &&
                (all_s || CurrentObservedVertices.count(CAST_ILV(y))))
                operation(sTPath, y->getId(), 1);
        }
    }

    for (auto const &a : tail->getEdges()) {
        auto x = CAST_ILV(a->getOther(tail));
        if (x == head)
            continue;
        if (already_observed.find(x->getId()) == already_observed.end() &&
            (all_s || CurrentObservedVertices.count(x))) {
            operation(sTPath, x->getId(),
                      sTPathCount(head, tail, x, !headLow, !tailLow));
            already_observed.emplace(x->getId(), std::monostate{});
        }

        for (auto const &a2 : x->getEdges()) {
            auto y = CAST_ILV(a2->getOther(x));

            if (y != head && y != tail &&
                (all_s || CurrentObservedVertices.count(CAST_ILV(y))))
                operation(sTPath, y->getId(), 1);
        }
    }

    already_observed.clear();
}

/**
 * @brief Changes the Claw S-Count for all or the specified vertices if the arc
 * a gets inserted or deleted. If a gets inserted, the function "operation"
 * should increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sClawArcChange(const Algora::Arc *a,
                                     IncOrDecSingleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    int deg_h = graph->getUndirectedDegree(head) - 1;
    int deg_t = graph->getUndirectedDegree(tail) - 1;

    for (auto s : this->CurrentObservedVertices) {
        operation(sClaw, s.first->getId(),
                  sClawCount(head, tail, s.first, deg_h, deg_t));
    }
    if (!observedVertices.empty()) {
        return;
    }

    already_observed.clear();

    operation(sClaw, head->getId(), sClawCount(head, tail, head, deg_h, deg_t));
    already_observed.emplace(head->getId(), std::monostate{});
    operation(sClaw, tail->getId(), sClawCount(head, tail, tail, deg_h, deg_t));
    already_observed.emplace(tail->getId(), std::monostate{});
    for (auto const &a1 : head->getEdges()) {
        auto s = CAST_ILV(a1->getOther(head));
        if (already_observed.find(s->getId()) == already_observed.end()) {
            operation(sClaw, s->getId(),
                      sClawCount(head, tail, s, deg_h, deg_t));
            already_observed.emplace(s->getId(), std::monostate{});
        }
    }

    for (auto const &a1 : tail->getEdges()) {
        auto s = CAST_ILV(a1->getOther(tail));
        if (already_observed.find(s->getId()) == already_observed.end()) {
            operation(sClaw, s->getId(),
                      sClawCount(head, tail, s, deg_h, deg_t));
            already_observed.emplace(s->getId(), std::monostate{});
        }
    }
    already_observed.clear();
}

/**
 * @brief Changes the Paw S-Count for all or the specified vertices if the arc a
 * gets inserted or deleted. If a gets inserted, the function "operation" should
 * increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sPawArcChange(Algora::Arc *a,
                                    IncOrDecSingleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    bool arc_removed = !edge_exist(head, tail);
    int tHeadTail = this->getNrTriangleWithEdge(head, tail);
    int tHead{0}, tTail{0};
    bool headHigh{is_high(head)};
    bool tailHigh{is_high(tail)};
    if (arc_removed) {
        tHead = getNrTriangle_pretended(head, head, tail);
        tTail = getNrTriangle_pretended(tail, head, tail);
    } else {
        tHead = getNrTriangle(head);
        tTail = getNrTriangle(tail);
    }

    already_observed.clear();
    operation(sPaw, head->getId(),
              sPawCount(head, tail, head, headHigh, tailHigh, arc_removed,
                        tHead, tTail, tHeadTail));
    already_observed.emplace(head->getId(), std::monostate{});
    operation(sPaw, tail->getId(),
              sPawCount(head, tail, tail, headHigh, tailHigh, arc_removed,
                        tHead, tTail, tHeadTail));
    already_observed.emplace(tail->getId(), std::monostate{});

    for (auto const &a : head->getEdges()) {
        auto x = CAST_ILV(a->getOther(head));

        if (already_observed.find(x->getId()) == already_observed.end() &&
            (all_s || CurrentObservedVertices.count(x))) {
            operation(sPaw, x->getId(),
                      sPawCount(head, tail, x, headHigh, tailHigh, arc_removed,
                                tHead, tTail, tHeadTail));
            already_observed.emplace(x->getId(), std::monostate{});
        }

        if (!edge_exist(x, tail))
            continue;
        for (auto const &a : x->getEdges()) {
            auto y = CAST_ILV(a->getOther(x));
            if (y != head && y != tail &&
                (all_s || CurrentObservedVertices.count(y)))
                operation(sPaw, y->getId(), 1);
        }
    }

    for (auto const &a : tail->getEdges()) {
        auto x = CAST_ILV(a->getOther(tail));

        if (already_observed.find(x->getId()) == already_observed.end() &&
            (all_s || CurrentObservedVertices.count(x))) {
            operation(sPaw, x->getId(),
                      sPawCount(head, tail, x, headHigh, tailHigh, arc_removed,
                                tHead, tTail, tHeadTail));
            already_observed.emplace(x->getId(), std::monostate{});
        }
    }

    already_observed.clear();
}

/**
 * @brief Changes the Four-Cycle S-Count for all or the specified vertices if
 * the arc a gets inserted or deleted. If a gets inserted, the function
 * "operation" should increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sFCycleArcChange(Algora::Arc *a,
                                       IncOrDecSingleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool headHigh{is_high(head)};
    bool tailHigh{is_high(tail)};
    already_observed.clear();

    operation(sFCycle, head->getId(),
              sFCycleCount(head, tail, head, headHigh, tailHigh));
    already_observed.emplace(head->getId(), std::monostate{});

    operation(sFCycle, tail->getId(),
              sFCycleCount(head, tail, tail, headHigh, tailHigh));
    already_observed.emplace(tail->getId(), std::monostate{});

    if (!headHigh) {
        for (auto const &a : head->getEdges()) {
            auto x = CAST_ILV(a->getOther(head));
            if (x == tail)
                continue;

            if (already_observed.find(x->getId()) == already_observed.end() &&
                (all_s || CurrentObservedVertices.count(x))) {
                operation(sFCycle, x->getId(),
                          sFCycleCount(head, tail, x, headHigh, tailHigh));
                already_observed.emplace(x->getId(), std::monostate{});
            }

            for (auto const &a : x->getEdges()) {
                auto y = CAST_ILV(a->getOther(x));
                if (!edge_exist(y, tail))
                    continue;
                if (y != head && y != tail &&
                    (all_s || CurrentObservedVertices.count(y)))
                    operation(sFCycle, y->getId(), 1);
            }
        }
    } else {
        for (auto const &a1 : head->getEdges()) {
            auto s = CAST_ILV(a1->getOther(head));
            if (already_observed.find(s->getId()) == already_observed.end() &&
                (all_s || CurrentObservedVertices.count(s))) {
                operation(sFCycle, s->getId(),
                          sFCycleCount(head, tail, s, headHigh, tailHigh));
                already_observed.emplace(s->getId(), std::monostate{});
            }
        }
    }
    if (!tailHigh) {
        for (auto const &a : tail->getEdges()) {
            auto x = CAST_ILV(a->getOther(tail));

            if (already_observed.find(x->getId()) == already_observed.end() &&
                (all_s || CurrentObservedVertices.count(x))) {
                operation(sFCycle, x->getId(),
                          sFCycleCount(head, tail, x, headHigh, tailHigh));
                already_observed.emplace(x->getId(), std::monostate{});
            }

            if (x == head)
                continue;
            for (auto const &a : x->getEdges()) {
                auto y = CAST_ILV(a->getOther(x));
                if (!edge_exist(y, head))
                    continue;
                if (y != head && y != tail &&
                    (all_s || CurrentObservedVertices.count(y)))
                    operation(sFCycle, y->getId(), 1);
            }
        }
    } else {
        for (auto const &a1 : tail->getEdges()) {
            auto s = CAST_ILV(a1->getOther(tail));
            if (already_observed.find(s->getId()) == already_observed.end() &&
                (all_s || CurrentObservedVertices.count(s))) {
                operation(sFCycle, s->getId(),
                          sFCycleCount(head, tail, s, headHigh, tailHigh));
                already_observed.emplace(s->getId(), std::monostate{});
            }
        }
    }

    already_observed.clear();
}

/**
 * @brief Changes the Diamond S-Count for all or the specified vertices if the
 * arc a gets inserted or deleted. If a gets inserted, the function "operation"
 * should increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sDiamondArcChange(Algora::Arc *a,
                                        IncOrDecSingleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool headHigh{is_high(head)};
    bool tailHigh{is_high(tail)};
    already_observed.clear();

    operation(sDiamond, head->getId(),
              sDiamondCount(head, tail, head, headHigh, tailHigh));
    already_observed.emplace(head->getId(), std::monostate{});

    operation(sDiamond, tail->getId(),
              sDiamondCount(head, tail, tail, headHigh, tailHigh));
    already_observed.emplace(tail->getId(), std::monostate{});

    for (auto const &a : head->getEdges()) {
        auto x = CAST_ILV(a->getOther(head));
        if (already_observed.find(x->getId()) == already_observed.end() &&
            (all_s || CurrentObservedVertices.count(x))) {
            operation(sDiamond, x->getId(),
                      sDiamondCount(head, tail, x, headHigh, tailHigh));
            already_observed.emplace(x->getId(), std::monostate{});
        }
        if (!edge_exist(x, tail))
            continue;
        for (auto const &a : x->getEdges()) {
            auto y = CAST_ILV(a->getOther(x));
            if (y != head && y != tail &&
                (all_s || CurrentObservedVertices.count(y))) {
                if (edge_exist(y, head))
                    operation(sDiamond, y->getId(), 1);
                if (edge_exist(y, tail))
                    operation(sDiamond, y->getId(), 1);
            }
        }
    }

    for (auto const &a : tail->getEdges()) {
        auto x = CAST_ILV(a->getOther(tail));
        if (already_observed.find(x->getId()) == already_observed.end() &&
            (all_s || CurrentObservedVertices.count(x))) {
            operation(sDiamond, x->getId(),
                      sDiamondCount(head, tail, x, headHigh, tailHigh));
            already_observed.emplace(x->getId(), std::monostate{});
        }
    }

    already_observed.clear();
}

/**
 * @brief Changes the Four-Clique S-Count for all or the specified vertices if
 * the arc a gets inserted or deleted. If a gets inserted, the function
 * "operation" should increase the count and decrease it if a gets removed
 *
 * @param a Arc that gets inserted or deleted
 * @param operation Operation that increases the count if the arc a gets
 * inserted and reduces the count otherwise
 */
void HSubgraphCounts::sFCliqueArcChange(Algora::Arc *a,
                                        IncOrDecSingleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool headHigh{is_high(head)};
    bool tailHigh{is_high(tail)};

    for (auto s : this->CurrentObservedVertices) {
        operation(sFClique, s.first->getId(),
                  sFCliqueCount(head, tail, s.first, headHigh, tailHigh));
    }
    if (!observedVertices.empty()) {
        return;
    }

    operation(sFClique, head->getId(),
              sFCliqueCount(head, tail, head, headHigh, tailHigh));
    operation(sFClique, tail->getId(),
              sFCliqueCount(head, tail, tail, headHigh, tailHigh));
    for (auto const &a1 : head->getEdges()) {
        auto s = CAST_ILV(a1->getOther(head));
        if (s == tail)
            continue;
        operation(sFClique, s->getId(),
                  sFCliqueCount(head, tail, s, headHigh, tailHigh));
    }
}
