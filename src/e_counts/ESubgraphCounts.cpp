#include "e_counts/ESubgraphCounts.h"
#include "ESubgraphCounts.h"
#include "graph/arc.h"
#include "graph/vertex.h"
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <cmath>
#include <iostream>
#include <vector>

/**
 * @brief prepares the algorithm by introducing all necessary observers
 * according to the set options. First, the observers for maintaining the
 * auxiliary counts when an arc is removed are set. Then the ones that calculate
 * the changes in the subgraph counts. Finally the ones that maintain the
 * auxiliary counts when an arc in added are set.
 *
 * This ensures the correct order of operations.
 */
bool ESubgraphCounts::prepare() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before preparing.\n";
        return 0;
    }

    // Here, the dependencies between the subgraph counts and the necessary
    // auxiliary counts are defined.
    bool s0{objectives[1]};
    bool s1{objectives[3]};
    bool s2{objectives[5] || objectives[1] || objectives[3] || objectives[6] ||
            objectives[0]};
    bool s3{objectives[4]};
    bool s4{objectives[3]};
    bool s5{objectives[5]};
    bool s6{objectives[6]};
    bool s7{objectives[6] || objectives[5] || objectives[3]};
    StrucCount.setGraph(graph);
    StrucCount.initVertexPartition();
    StrucCount.setObjectives(s0, s1, s2, s3, s4, s5, s6, s7);

    // Change of aux-counts for removal
    StrucCount.vPart->onArcRemove(
        &StrucCount.OnEpsArcRemoveId.emplace_back(15), [&](Algora::Arc *a) {
            last_arc = a;
#ifdef TRACK_CACHE
            cacheMisses.start();
#endif
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
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
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            delTimeGraph += end - start;
#endif
#ifdef TRACK_CACHE
            cacheMisses.stop();
#endif
        });

    // Change in subgraph counts for insertion and removal
    StrucCount.prepare();

    // Change of aux-counts for insertion
    StrucCount.vPart->onArcAdd(
        &StrucCount.OnEpsArcAddId.emplace_back(15), [&](Algora::Arc *a) {
            last_arc = a;
#ifdef TRACK_CACHE
            cacheMisses.start();
#endif
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
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
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTimeGraph += end - start;
#endif
#ifdef TRACK_CACHE
            cacheMisses.stop();
#endif
        });

    return 1;
}

void ESubgraphCounts::unsetGraph() { StrucCount.vPart->unsetGraph(); }

/**
 * @brief counts the changes in the total number of two-edges after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
// void ESubgraphCounts::tEdgeArcChange(const Algora::Arc *a,
//                                      CounterUpdate *operation) {
//     const Algora::Vertex *head = a->getHead();
//     const Algora::Vertex *tail = a->getTail();

//     int increment = (graph->getNumArcs(1) - 1) -
//                     (graph->getUndirectedDegree(head) +
//                      graph->getUndirectedDegree(tail) - 2);
//     operation(tEdges, increment);
// }

/**
 * @brief counts the changes in the total number of four-clique after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::fCliqueArcChange(const Algora::Arc *a,
                                       CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_high{is_high(head)};
    bool tail_high{is_high(tail)};
    COUNTER_TYPE increment{0};

    if (head_high && tail_high) {
        increment += StrucCount.getNrs6(head, tail);
        forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            if (!edge_exist(h, tail)) {
                return;
            }

            increment += StrucCount.getNrs7(head, tail, h);
            forEachHighNeighbor(head, {tail}, h, [&](auto h2) {
#if ENABLE_DEEPER_STATS
                ++iterate_h;
#endif
                if (edge_exist(h2, tail) && edge_exist(h2, h))
                    increment++;
            });
        });

    } else {
        StrucCount.forLowerDegree(head, tail, [&](auto low, auto any) {
            forEachNeighbor(low, {any}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (!edge_exist(x, any))
                    return;
                forEachNeighbor(low, {any}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    if (edge_exist(y, any) && edge_exist(y, x))
                        increment++;
                });
            });
        });
    }

    operation(fClique, increment);
}

/**
 * @brief counts the changes in the total number of claws after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::ClawArcChange(const Algora::Arc *a,
                                    CounterUpdate *operation)

{
    const Algora::Vertex *head = a->getHead();
    const Algora::Vertex *tail = a->getTail();

    COUNTER_TYPE d1 = graph->getUndirectedDegree(head);
    COUNTER_TYPE d2 = graph->getUndirectedDegree(tail);
    COUNTER_TYPE increment{0};
    if (d1 > 2)
        increment += boost::math::binomial_coefficient<double>(d1 - 1, 2);
    if (d2 > 2)
        increment += boost::math::binomial_coefficient<double>(d2 - 1, 2);

    operation(claw, increment);
}

/**
 * @brief counts the changes in the total number of diamonds after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::DiamondArcChange(const Algora::Arc *a,
                                       CounterUpdate *operation) {
    auto *head = CAST_ILV(a->getHead());
    auto *tail = CAST_ILV(a->getTail());
    bool head_high{is_high(head)};
    bool tail_high{is_high(tail)};

    auto increment{0};
    if (head_high && tail_high) {
        // 1. Count triangles to find diamonds with head - tail on diagonal
        // 2. Check for head-tail on outer circle

        // 1. Triangles: (a) third is low
        COUNTER_TYPE triangles = StrucCount.getNrs2(head, tail);

        forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            // 2. Outer circle: (b) head - tail  - h - low, low - head/tail
            // diagonal
            increment += StrucCount.getNrs7(head, tail, h);

            if (edge_exist(h, tail)) {
                // 1. Triangles: (b) third is high
                ++triangles;
                // 2. Outer circle: (a) head - tail  - h - low, h - head/tail
                // diagonal
                increment += StrucCount.getNrs2(h, head);
                increment += StrucCount.getNrs2(h, tail);
            }
        });

        forEachHighNeighbor(tail, {head}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            increment += StrucCount.getNrs7(head, tail, h);
        });

        // 2. Outer circle: (b) head - tail  - low - low, low - head/tail
        // diagonal
        increment += StrucCount.getNrs5(head, tail);

        // 1. Triangles: Get count from triangles
        if (triangles > 1) {
            increment += triangles * (triangles - 1) / 2;
        }
    } else {
        // Use the low degree vertex or the lower degree id both are low
        StrucCount.vPart->forLowLowerDegree(head, tail, [&](auto low, auto v) {
            auto v_is_high{is_high(v)};

            StrucCount.forEachNeighbor(low, {v}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                auto x_v = edge_exist(x, v);
                auto x_is_high = is_high(x);

                // 1. low is part of the diagonal
                StrucCount.forEachNeighbor(low, {v, x}, x, [&](auto w) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    auto w_is_high = is_high(w);

                    auto w_v = edge_exist(w, v);
                    auto w_x = edge_exist(w, x);
                    // low - v/w is diagonal
                    increment += (x_v && w_v && (x->getId() < w->getId())) +
                                 ((!x_is_high || !w_is_high) && w_x && w_v);
                });

                // 2. low is NOT part of the diagonal (x - v is diagonal)
                if (!x_v)
                    return;

                if (v_is_high && x_is_high) {
                    increment += StrucCount.getNrs2(x, v) - 1;
                } else {
                    COUNTER_TYPE dx = x->getUndirectedDegree();
                    COUNTER_TYPE dv = v->getUndirectedDegree();
                    auto *v1 = dx > dv ? v : x;
                    auto *v2 = dx > dv ? x : v;

                    // Avoid double counts with updateAllHigh, only count if
                    // x or w is low
                    if (!x_is_high) {
                        StrucCount.forEachNeighbor(v1, {v2, low}, [&](auto w) {
#if ENABLE_DEEPER_STATS
                            ++iterate_neighbor;
#endif
                            increment += edge_exist(w, v2);
                        });
                    } else {
                        StrucCount.forEachLowNeighbor(
                            v1, {v2, low}, nullptr, [&](auto w) {
#if ENABLE_DEEPER_STATS
                                ++iterate_neighbor;
#endif
                                increment += edge_exist(w, v2);
                            });
                    }
                }
            });
        });
    }

    // 2. Outer circle: (a) head - tail  - h1 - h2, h1 - head/tail
    // diagonal
    auto updateAllHigh = [&](auto v1, auto v2) {
        StrucCount.forEachTwoHighNeighbors(v1, {v2}, [&](ILV *h1, ILV *h2) {
#if ENABLE_DEEPER_STATS
            iterate_h += 2;
#endif
            auto h1_h2 = edge_exist(h2, h1);
            if (!h1_h2)
                return;
            auto h1_v2 = edge_exist(v2, h1);
            auto h2_v2 = edge_exist(v2, h2);
            increment += h1_v2 + h2_v2;
        });
    };
    updateAllHigh(head, tail);
    updateAllHigh(tail, head);

    operation(diamond, increment);
}

/**
 * @brief counts the changes in the total number of three-paths after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::tPathArcChange(const Algora::Arc *a,
                                     CounterUpdate *operation) {
    auto *head = CAST_ILV(a->getHead());
    auto *tail = CAST_ILV(a->getTail());
    COUNTER_TYPE d1 = head->getUndirectedDegree();
    COUNTER_TYPE d2 = tail->getUndirectedDegree();
    bool head_high{is_high(head)};
    bool tail_high{is_high(tail)};

    // 1. additional edge is the center in the three-edge path
    COUNTER_TYPE increment = (d1 - 1) * (d2 - 1);
    // Remove double counts from common neighbors
    if (head_high && tail_high) {
        increment -= StrucCount.getNrs2(head, tail);

        StrucCount.forEachHighNeighbor(head, {tail}, nullptr, [&](auto highV) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            increment -= edge_exist(highV, tail);
        });
    } else {
        StrucCount.forLowerDegree(head, tail, [&](auto ldv, auto hdv) {
            StrucCount.forEachNeighbor(ldv, {hdv}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                increment -= edge_exist(x, hdv);
            });
        });
    }

    // 2. additional edge is the not center in the three-edge path
    // Assume wlog: v2 - v1 - _ - _
    auto updateForDirection = [&](auto v1, auto v2) {
        if (is_high(v1)) {
            StrucCount.forEachHighV({v1, v2}, nullptr, [&](auto highV) {
#if ENABLE_DEEPER_STATS
                ++iterate_h;
#endif
                if (edge_exist(highV, v1)) {
                    // v2 - v1 - H - any
                    increment += (highV->getUndirectedDegree() - 1) -
                                 edge_exist(highV, v2);
                }

                // v2 - v1 - low - H
                increment += StrucCount.getNrs2(v1, highV) -
                             (StrucCount.getNrs2(v1, highV) && is_low(v2) &&
                              edge_exist(highV, v2));
            });
            // v2 - v1 - low - low
            increment += StrucCount.getNrs0(v1);
            // v2 - v1 - low - low: Remove double counts + triangles
            if (StrucCount.getNrs0(v1) && is_low(v2)) {
                StrucCount.forEachLowNeighbor(v2, {}, nullptr, [&](auto &lowV) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    increment -= 1 + edge_exist(lowV, v1);
                });
            }
        } else {
            // Only has <=h neighbors -> check for each!
            StrucCount.forEachNeighbor(v1, {v2}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                increment +=
                    (x->getUndirectedDegree() - 1) - edge_exist(x, v2);
            });
        }
    };

    updateForDirection(head, tail);
    updateForDirection(tail, head);

    operation(tPath, increment);
}

/**
 * @brief counts the changes in the total number of paws after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::pawArcChange(const Algora::Arc *a,
                                   CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_high{is_high(head)};
    bool tail_high{is_high(tail)};
    COUNTER_TYPE d1 = head->getUndirectedDegree();
    COUNTER_TYPE d2 = tail->getUndirectedDegree();

    COUNTER_TYPE increment{0};
    bool done{false};
    std::vector<ILV *> eligibleNeighbors;

    // 1. New edge is arm: Find triangle x-y-v, combined with v-other this is
    // a paw!
    auto findTriangleAroundV = [&](auto v, auto other) {
        bool v_high{is_high(v)};
        bool other_high{is_high(other)};

        if (v_high) {
            COUNTER_TYPE zw = StrucCount.getNrs1(v);
            if (zw != 0 && !other_high) {
                forEachLowNeighbor(other, {v}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    if (edge_exist(x, v))
                        zw--;
                });
            }
            increment += zw;

            forEachHighNeighbor(v, {other}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
                ++iterate_h;
#endif
                increment += StrucCount.getNrs2(v, h);
                if (!other_high && edge_exist(h, other))
                    increment--;
                forEachHighNeighbor(v, {other}, h, [&](auto h2) {
#if ENABLE_DEEPER_STATS
                    ++iterate_h;
#endif
                    if (edge_exist(h2, h))
                        ++increment;
                });
            });
        } else {
            eligibleNeighbors.clear();
            forEachNeighbor(v, {other}, [&](auto x) {
                eligibleNeighbors.push_back(x);
            });
            for (size_t i = 0; i < eligibleNeighbors.size(); ++i) {
                auto x = eligibleNeighbors[i];
                for (size_t j = 0; j < i; ++j) {
                    auto y = eligibleNeighbors[j];
#if ENABLE_DEEPER_STATS
                    iterate_neighbor += 2;
#endif
                    if (edge_exist(x, y))
                        ++increment;
                }
            }
        }
    };

    findTriangleAroundV(head, tail);
    findTriangleAroundV(tail, head);

    COUNTER_TYPE nr_t{0};

    // 2. New edge is part of the triangle

    if (head_high && tail_high) {
        nr_t = StrucCount.getNrs2(head, tail);
        StrucCount.forEachHighV({head, tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            increment += StrucCount.getNrs7(head, tail, h);

            if (edge_exist(h, head) && edge_exist(h, tail)) {
                nr_t++;
                increment += h->getUndirectedDegree() - 2;
            }
        });
        increment += StrucCount.getNrs4(head, tail);
    } else {
        StrucCount.forLowerDegree(head, tail, [&](auto low, auto other) {
            forEachNeighbor(low, {}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (edge_exist(x, other)) {
                    increment += x->getUndirectedDegree() - 2;
                    nr_t += 1;
                }
            });
        });
    }

    increment += nr_t * (d1 - (2));
    increment += nr_t * (d2 - (2));

    operation(paw, increment);
}

/**
 * @brief counts the changes in the total number of four-cycles after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::fCycleArcChange(const Algora::Arc *a,
                                      CounterUpdate *operation) {
    const auto head = CAST_ILV(a->getHead());
    const auto tail = CAST_ILV(a->getTail());
    auto head_high = is_high(head);
    auto tail_high = is_high(tail);

    COUNTER_TYPE increment{0};

    // 1. Both middle vertices are low!
    if (head_high && tail_high || !StrucCount_has_s3_HighAnchors) {
        increment += StrucCount.getNrs3(head, tail);
    } else {
        // Find 4Cycles with 2 low middle vertices
        // Always iterate neighbors of the lower degree vertex
        StrucCount.forLowerDegree(head, tail, [&](auto ldv, auto hdv) {
            StrucCount.forEachLowNeighbor(ldv, {hdv}, nullptr, [&](auto l1) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                StrucCount.forLowerDegree(l1, hdv, [&](auto ldv2, auto hdv2) {
                    StrucCount.forEachLowNeighbor(
                        ldv2, {hdv2, ldv}, nullptr, [&](auto l2) {
#if ENABLE_DEEPER_STATS
                            ++iterate_neighbor;
#endif
                            // ldv - ldv2 - l2 - hdv2
                            increment += edge_exist(l2, hdv2);
                        });
                });
            });
        });
    }

    // 2. At least one middle vertex is high!
    auto updateInDirectionV = [&](auto v, auto other, auto count_both_high) {
        forEachHighNeighbor(v, {other}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif

            if (is_high(other)) {
                increment += StrucCount.getNrs2(other, h);
                if (is_low(v))
                    increment--;
            } else {
                forEachLowNeighbor(other, {v}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    if (edge_exist(x, h)) {
                        increment++;
                    }
                });
            }
            if (count_both_high) {
                forEachHighNeighbor(other, {v, h}, nullptr, [&](auto h2) {
#if ENABLE_DEEPER_STATS
                    ++iterate_h;
#endif
                    if (edge_exist(h, h2)) {
                        increment++;
                    }
                });
            }
        });
    };

    updateInDirectionV(head, tail, 1);
    updateInDirectionV(tail, head, 0);
    operation(fCycle, increment);
}

/**
 * @brief counts the changes in the total number of threes-cycles after the
 * insertion/deletion of given arc. This change is then given to the given
 * function that should either increase or decrease the cout by that number
 * depending on if the arc was inserted or deleted
 *
 * @param a arc that was inserted/deleted
 * @param operation
 */
void ESubgraphCounts::tCycleArcChange(const Algora::Arc *a,
                                      CounterUpdate *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_high{is_high(head)};
    bool tail_high{is_high(tail)};

    COUNTER_TYPE increment{0};

    if (head_high && tail_high) {
        forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            if (edge_exist(h, tail))
                increment++;
        });
        increment += StrucCount.getNrs2(head, tail);
    } else {
        StrucCount.forLowerDegree(head, tail, [&](auto low, auto other) {
            forEachNeighbor(low, {other}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (edge_exist(x, other))
                    increment++;
            });
        });
    }

    operation(tCycle, increment);
}
