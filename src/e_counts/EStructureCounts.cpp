#include "e_counts/EStructureCounts.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph/arc.h"
#include "graph/graph_functional.h"
#include "graph/vertex.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <iostream>
#include <vector>
/**
 * @brief Create the partition that divides the vertices into low and high
 * degree using the h-index
 *
 * @return true
 * @return false
 */
bool EStructureCounts::initVertexPartition() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before creation Epsilon Table\n";
        return 0;
    }
    vPart->cleanup();
    vPart->setGraph(graph);
    if (!vPart->prepare())
        return 0;

    vPart->setOnRecompute(
        [&](bool arcAdd,
            boost::unordered_map<const Algora::Vertex *, int> &arcBorder) {
            this->cleanMaps();
            this->recompute(arcAdd, arcBorder);
        });
    return 1;
}

bool EStructureCounts::recompute(
    bool arcAdd, boost::unordered_map<const Algora::Vertex *, int> &arcBorder) {

    vPart->fixTable();

    std::vector<Algora::Arc *> arcs;
    arcs.reserve(vPart->get_m());

    graph->mapEdges([&](auto e) {
        if (e == vPart->getRemovedArc())
            return;
        arcs.push_back(e);
    });

    for (auto it = arcs.begin(); it != arcs.end(); it++) {
        graph->deactivateEdge(*it);
    }

    vPart->adjacencyDeleteAll();

    for (auto it = arcs.begin(); it != arcs.end(); it++) {

        graph->activateEdge(*it);

        auto head = CAST_ILV((*it)->getHead());
        auto tail = CAST_ILV((*it)->getTail());

        vPart->adjacencyInsert(*it);

        if (is_high(head)) {
            tail->swapEdges(tail->undirectedIndexOf(*it), arcBorder[tail]++);
        }
        if (is_high(tail)) {
            head->swapEdges(head->undirectedIndexOf(*it), arcBorder[head]++);
        }

        ArcChange(*it, increaseOrCreate, increaseOrCreate, increaseOrCreate);
    }

    vPart->unfixTable();

    return 1;
}

/**
 * @brief Set the objectives.
 * This means define which auxiliary counts need to be maintained.
 *
 */
void EStructureCounts::setObjectives(bool s0, bool s1, bool s2, bool s3,
                                     bool s4, bool s5, bool s6, bool s7) {

    count_s[0] = s0;
    count_s[1] = s1;
    count_s[2] = s2;
    count_s[3] = s3;
    count_s[4] = s4;
    count_s[5] = s5;
    count_s[6] = s6;
    count_s[7] = s7;
}

/**
 * @brief Prepare by creating all observers of the given graph that maintain the
 * auxiliary counts. That includes whenever an arc is added/removed and
 * whenever a vertex changes partition
 *
 */
bool EStructureCounts::prepare() {

    if (!hasGraph() || !vPart->hasGraph()) {
        std::cerr << "Need to set Graph and EpsilonTable before preparing\n";
        return 0;
    }

    vPart->onArcAdd(&OnEpsArcAddId.emplace_back(0), [&](Algora::Arc *a) {
#ifdef TRACK_CACHE
        cacheMisses.start();
#endif
#if ENABLE_DEEPER_STATS
        auto start = std::chrono::high_resolution_clock::now();
#endif

        DEBUG_PRINT("onArcAdd (%d,%d)", a->getFirst()->getId(),
                    a->getSecond()->getId());
        this->ArcChange(a, increaseOrCreate, increaseOrCreate,
                        increaseOrCreate);

#if ENABLE_DEEPER_STATS
        auto end = std::chrono::high_resolution_clock::now();
        addTime += end - start;
#endif
#ifdef TRACK_CACHE
        cacheMisses.stop();
#endif
    });

    vPart->onArcRemove(&OnEpsArcRemoveId.emplace_back(0), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
        auto start = std::chrono::high_resolution_clock::now();
#endif
#ifdef TRACK_CACHE
        cacheMisses.start();
#endif
        DEBUG_PRINT("onArcRemove (%d,%d)", a->getFirst()->getId(),
                    a->getSecond()->getId());
        this->ArcChange(a, reduceOrDelete, reduceOrDelete, reduceOrDelete);

#if ENABLE_DEEPER_STATS
        auto end = std::chrono::high_resolution_clock::now();
        delTime += end - start;
#endif
#ifdef TRACK_CACHE
        cacheMisses.stop();
#endif
    });

    vPart->onBeforeVertexToHigh(
        &OnVertexUpId.emplace_back(0), [&](Algora::Vertex *v) {
#ifdef TRACK_CACHE
            cacheMissesVertex.start();
#endif
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif

            DEBUG_PRINT("onBeforeVertexToHigh (%d)", v->getId(),
                        directVertexChange);

            if (directVertexChange) {
                this->VertexChange(v, increaseOrCreate, reduceOrDelete,
                                   increaseOrCreate, reduceOrDelete,
                                   increaseOrCreate, reduceOrDelete);
            } else {
                // remove from all structures where it was part
                this->VertexChange(v, reduceOrDelete, reduceOrDelete,
                                   reduceOrDelete);
            }

#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            toHighTime += end - start;
#endif
#ifdef TRACK_CACHE
            cacheMissesVertex.stop();
#endif
        });

    vPart->onVertexToHigh(
        &OnVertexUpId.emplace_back(0), [&](Algora::Vertex *v) {
#ifdef TRACK_CACHE
            cacheMissesVertex.start();
#endif
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif

            DEBUG_PRINT("onVertexToHigh (%d)", v->getId());

            if (directVertexChange) {
                // nothing to do
            } else {
                // add to all structures where it is part now
                this->VertexChange(v, increaseOrCreate, increaseOrCreate,
                                   increaseOrCreate);
            }

#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            toHighTime += end - start;
#endif
#ifdef TRACK_CACHE
            cacheMissesVertex.stop();
#endif
        });

    vPart->onBeforeVertexToLow(
        &OnVertexDownId.emplace_back(0), [&](Algora::Vertex *v) {
#ifdef TRACK_CACHE
            cacheMissesVertex.start();
#endif
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif

            DEBUG_PRINT("onBeforeVertexToLow (%d)", v->getId());

            if (directVertexChange) {
                this->VertexChange(v, reduceOrDelete, increaseOrCreate,
                                   reduceOrDelete, increaseOrCreate,
                                   reduceOrDelete, increaseOrCreate);
            } else {
                // remove from all structures where it was part
                this->VertexChange(v, reduceOrDelete, reduceOrDelete,
                                   reduceOrDelete);
            }

#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            toLowTime += end - start;
#endif
#ifdef TRACK_CACHE
            cacheMissesVertex.stop();
#endif
        });

    vPart->onVertexToLow(
        &OnVertexDownId.emplace_back(0), [&](Algora::Vertex *v) {
#ifdef TRACK_CACHE
            cacheMissesVertex.start();
#endif
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif

            DEBUG_PRINT("onVertexToLow (%d)", v->getId());

            if (directVertexChange) {
                // nothing to do
            } else {
                // add to all structures where it is part now
                this->VertexChange(v, increaseOrCreate, increaseOrCreate,
                                   increaseOrCreate);
            }

#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            toLowTime += end - start;
#endif
#ifdef TRACK_CACHE
            cacheMissesVertex.stop();
#endif
        });

    return 1;
}

/**
 * @brief Clear all auxiliary counts
 *
 */
void EStructureCounts::cleanMaps() {
    this->s0.clear();
    this->s1.clear();
    this->s2.clear();
    this->s3.clear();
    this->s4.clear();
    this->s5.clear();
    this->s6.clear();
    this->s7.clear();
}
/**
 * @brief Remove all added observers
 *
 */
void EStructureCounts::cleanup() {
    cleanMaps();
    for (auto &id : OnVertexUpId)
        vPart->removeOnVertexToHigh(&id);
    for (auto &id : OnVertexDownId)
        vPart->removeOnVertexToLow(&id);
    for (auto &id : OnEpsArcAddId)
        vPart->removeOnArcAdd(&id);
    for (auto &id : OnEpsArcRemoveId)
        vPart->removeOnArcRemove(&id);
}

/**
 * @brief This function calculates the changes of all auxiliary counts when an
 * edge is inserted or deleted. It then provides this change to the given
 * functions that then increase or decrease the counts depending on if the edge
 * was removed or inserted.
 *
 *
 * @param a Arc that is inserted/deleted
 * @param operation1 function that changes the count of a auxiliary structure
 * that has one anchor vertex
 * @param operation2 function that changes the count of a auxiliary structure
 * that has two anchor vertex
 * @param operation3 function that changes the count of a auxiliary structure
 * that has three anchor vertex
 */
void EStructureCounts::ArcChange(const Algora::Arc *a,
                                 IncOrDecSingleKey *operation1,
                                 IncOrDecDoubleKey *operation2,
                                 IncOrDecTripleKey *operation3) {
    auto head = CAST_ILV(a->getHead());
    auto tail = CAST_ILV(a->getTail());
    bool head_low{is_low(head)};
    bool tail_low{is_low(tail)};

    // Note that all counts are only maintained for high deg vertices except for
    // s3!

    if (!head_low && !tail_low) {
        // no update needed
        return;
    }

    if (head_low && tail_low) {
        // 3path s3: 1. Added edge in middle; 2. Added edge at side
        if (count_s[3]) {
#ifdef TRACK_DEEP_TIME
            auto start = std::chrono::high_resolution_clock::now();
#endif
            if (highAnchorsOnlyS3) {
                // 1. w - head - tail - x ?
                forEachHighNeighbor(head, {tail}, nullptr, [&](auto w) {
                    forEachHighNeighbor(tail, {head, w}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
                        operation2(s3, w->getId(), x->getId(), 1);
                    });
                });
            } else {
                // 1. w - head - tail - x ?
                forEachNeighbor(head, {tail}, [&](auto w) {
                    forEachNeighbor(tail, {head, w}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
#ifdef TRACK_DEEP_TIME
                        auto start = std::chrono::high_resolution_clock::now();
#endif
                        operation2(s3, w->getId(), x->getId(), 1);
#if TRACK_DEEP_TIME
                        auto end = std::chrono::high_resolution_clock::now();
                        s3_Time += end - start;
#endif
                    });
                });
                // 2. v1 - v2 - low - x ?
                auto updateS3NotMiddle = [&](auto v1, auto v2) {
                    forEachLowNeighbor(v2, {v1}, nullptr, [&](auto low) {
                        forEachNeighbor(low, {v2, v1}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                            ++op_s3;
#endif
                            operation2(s3, v1->getId(), x->getId(), 1);
                        });
                    });
                };
                updateS3NotMiddle(head, tail);
                updateS3NotMiddle(tail, head);
            }
#if TRACK_DEEP_TIME
            auto end = std::chrono::high_resolution_clock::now();
            s3_Time += end - start;
#endif
        }

        if (!(count_s[0] || count_s[1] || count_s[4] || count_s[5] ||
              count_s[6])) {
            // No other counts to be updated
            return;
        }

        auto updateForV = [&](auto v, auto other, auto count_symmetric) {
            forEachHighNeighbor(v, {other}, nullptr, [&](auto h) {
                auto other_h = edge_exist(h, other);

                // 2path s0: h - head - tail or h - tail - head?
#if ENABLE_DEEPER_STATS
                ++op_s0;
#endif
                operation1(s0, h->getId(), 1);

                if (count_symmetric && other_h) {
                    // triangle s1: h - head - tail - h
#if ENABLE_DEEPER_STATS
                    ++op_s1;
#endif
                    operation1(s1, h->getId(), 1);
                }
                if (!(count_s[4] || count_s[5] || count_s[6])) {
                    // No other iteration needed
                    return;
                }
                forEachHighNeighbor(v, {other}, h, [&](auto h2) {
                    auto other_h2 = edge_exist(h2, other);

                    // claw s4: h - v - other - h2 ?
                    if (count_s[4]) {
#if ENABLE_DEEPER_STATS
                        ++op_s4;
#endif
                        operation2(s4, h->getId(), h2->getId(), 1);
                    }

                    // paw s5?
                    if (count_s[5] && other_h + other_h2 > 0) {

#if ENABLE_DEEPER_STATS
                        ++op_s5;
#endif
                        operation2(s5, h->getId(), h2->getId(),
                                   other_h + other_h2);
                    }

                    if (count_symmetric && count_s[6] &&
                        (other_h && other_h2)) {
                        // diamond s6?
#if ENABLE_DEEPER_STATS
                        ++op_s6;
#endif
                        operation2(s6, h->getId(), h2->getId(),
                                   other_h && other_h2);
                    }
                });
            });
        };

        updateForV(head, tail, 1);
        updateForV(tail, head, 0);
        return;
    }

    auto updateOneHighOneLow = [&](auto low, auto high) {
        // 2path s0: high-low-low?
        if (count_s[0]) {
#if ENABLE_DEEPER_STATS
            ++op_s0;
#endif
            operation1(s0, high->getId(),
                       vPart->getLowDegreeWithoutRemoved(low));
        }

        if (count_s[2] || count_s[4]) {
            forEachHighNeighbor(low, {high}, nullptr, [&](auto h) {
                // 2path s2: high - low - h ?
                if (count_s[2]) {
#if ENABLE_DEEPER_STATS
                    ++op_s2;
#endif
                    operation2(s2, high->getId(), h->getId(), 1);
                }

                // s4: claw?
                if (count_s[4]) {
#if ENABLE_DEEPER_STATS
                    ++op_s4;
#endif
                    operation2(s4, high->getId(), h->getId(),
                               vPart->getLowDegreeWithoutRemoved(low));
                }
            });
        }

        if (count_s[7]) {
            forEachTwoHighNeighbors(low, {high}, [&](auto h1, auto h2) {
#if ENABLE_DEEPER_STATS
                ++op_s7;
#endif
                // s7: claw?
                operation3(s7, high->getId(), h1->getId(), h2->getId(), 1);
            });
        }

        if (!(count_s[1] || count_s[3] || count_s[5] || count_s[6])) {
            // No other counts to update
            return;
        }

        forEachLowNeighbor(low, {}, nullptr, [&](auto l) {
        // triangle s1: high-low-low?
#if ENABLE_DEEPER_STATS
            ++op_s1;
#endif
            operation1(s1, high->getId(), edge_exist(l, high));

            if (count_s[3]) {
                if (highAnchorsOnlyS3) {
                    forEachHighNeighbor(l, {low, high}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
                        // s3: 3Path: high - low - l - x
                        operation2(s3, high->getId(), x->getId(), 1);
                    });
                } else {
                    forEachNeighbor(l, {low, high}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
                        // s3: 3Path: high - low - l - x
                        operation2(s3, high->getId(), x->getId(), 1);
                    });
                }
            }
            if (count_s[5] || count_s[6]) {
                forEachHighNeighbor(low, {high}, nullptr, [&](auto h) {
                    auto l_h = edge_exist(l, h);
                    auto l_high = edge_exist(l, high);

                    // s5: paw?
                    if (count_s[5] && l_h + l_high > 0) {

#if ENABLE_DEEPER_STATS
                        ++op_s5;
#endif
                        operation2(s5, high->getId(), h->getId(), l_h + l_high);
                    }

                    // s6: diamond?
                    if (count_s[6] && (l_h && l_high)) {

#if ENABLE_DEEPER_STATS
                        ++op_s6;
#endif
                        operation2(s6, high->getId(), h->getId(),
                                   l_h && l_high);
                    }
                });
            }

            if (count_s[5]) {
                forEachHighNeighbor(l, {high}, nullptr, [&](auto h) {
                    auto l_high = edge_exist(l, high);

                    // s5: paw?
#if ENABLE_DEEPER_STATS
                    ++op_s5;
#endif
                    operation2(s5, high->getId(), h->getId(), l_high);
                });
            }
        });
    };

    if (head_low && !tail_low) {
        updateOneHighOneLow(head, tail);
        return;
    } else {
        updateOneHighOneLow(tail, head);
    }
}

/**
 * @brief This function calculates the changes of all auxiliary counts when
 * a vertex changes partition. It then provides this change to the given
 * functions that then increase or decrease the counts depending on if the
 * vertex changes from high degree to low degree or the other way around.
 *
 *
 * @param v Vertex that is inserted/deleted
 * @param operation1 function that changes the count of a auxiliary
 * structure that has one anchor vertex
 * @param operation2 function that changes the count of a auxiliary
 * structure that has two anchor vertex
 * @param operation3 function that changes the count of a auxiliary
 * structure that has three anchor vertex
 */
void EStructureCounts::VertexChange(const Algora::Vertex *v,
                                    IncOrDecSingleKey *operation1,
                                    IncOrDecDoubleKey *operation2,
                                    IncOrDecTripleKey *operation3) {
    if (count_s[0] || count_s[1] || count_s[2] || count_s[3] || count_s[4] ||
        count_s[5] || count_s[6] || count_s[7]) {

        std::vector<Algora::Arc *> arcs;
        vPart->fixTable();
        graph->mapIncidentEdges(v, [&](Algora::Arc *a) { arcs.push_back(a); });

        for (auto a : arcs) {
            DEBUG_PRINT("Deactivate (%d,%d)", a->getFirst()->getId(),
                        a->getSecond()->getId());
            vPart->updateArcBorderOnRemove(a);
            graph->deactivateEdge(a);
            vPart->adjacencyDelete(a);
        }

        for (auto a : arcs) {
            DEBUG_PRINT("Reactivate (%d,%d)", a->getFirst()->getId(),
                        a->getSecond()->getId());
            graph->activateEdge(a);
            vPart->adjacencyInsert(a);
            vPart->updateArcBorderOnInsert(a);
            ArcChange(a, operation1, operation2, operation3);
        }

        vPart->unfixTable();
    }
}

// Update patterns s0..s7 around a center vertex v after a partition change by
// iterating over its low- and high-degree neighborhoods.
// If low->high: First parse increase than reduce operation for each key length
// If high->low: First parse reduce than increase operation for each key length
void EStructureCounts::VertexChange(Algora::Vertex *v_old,
                                    IncOrDecSingleKey *operation1a,
                                    IncOrDecSingleKey *operation1b,
                                    IncOrDecDoubleKey *operation2a,
                                    IncOrDecDoubleKey *operation2b,
                                    IncOrDecTripleKey *operation3a,
                                    IncOrDecTripleKey *operation3b) {

    auto v = CAST_ILV(v_old);
    auto v_low = vPart->is_low_deg(v);

    // Iterate over each low-degree neighbor l of v.
    forEachLowNeighbor(v, {}, nullptr, [&](auto l) {
        if (count_s[0]) {
            // v - l - low_deg(l)
            // vPart->getLowDegreeWithoutRemoved(l) returns low-degree of l
            // excluding the removed edge if incident, subtract v_low
            // (precomputed low-degree of v) and aggregate into s0
            operation1a(s0, v->getId(),
                        vPart->getLowDegreeWithoutRemoved(l) - v_low);
#if ENABLE_DEEPER_STATS
            ++op_s0;
#endif
        }
        if (count_s[0] || count_s[1] || count_s[2]) {
            forEachHighNeighbor(l, {v}, nullptr, [&](auto h) {
                // Pattern: l - v - h
                // Count presence of path v-l-h for s0
                if (count_s[0]) {
                    operation1b(s0, h->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s0;
#endif
                }

                // Additionally need h-v for s1
                if (count_s[1] && vPart->arcExists(h, v)) {
                    operation1b(s1, h->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s1;
#endif
                }
                // Count presence of path v-l-h for s2
                if (count_s[2]) {
                    operation2a(s2, h->getId(), v->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s2;
#endif
                }
            });
        }

        if (count_s[0]) {
            forEachHighNeighbor(v, {}, nullptr, [&](auto h) {
                // Pattern: l - v - h
                operation1b(s0, h->getId(), 1);
#if ENABLE_DEEPER_STATS
                ++op_s0;
#endif
            });
        }

        if (count_s[1] || count_s[5] || count_s[6]) {

            forEachLowNeighbor(v, {}, l, [&](auto l2) {
                // Pattern: l - v - l2
                if (count_s[1] && vPart->arcExists(l, l2)) {
                    operation1a(s1, v->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s1;
#endif
                }

                // Skip if there is no edge l-l2, higher-order motifs that rely
                // on the triangle
                if (!vPart->arcExists(l, l2)) {
                    return;
                }
                if (!(count_s[5] || count_s[6])) {
                    return;
                }
                // For each high neighbor of l (excluding v), we count
                // structures involving the triangle (l, v, l2)
                forEachHighNeighbor(l, {v}, nullptr, [&](auto h) {
                    // Triangle (l - v - l2) + extra edge l - h
                    if (count_s[5]) {

                        operation2a(s5, h->getId(), v->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_s5;
#endif
                    }
                    if (count_s[6] && vPart->arcExists(l2, h)) {

                        operation2a(s6, h->getId(), v->getId(),
                                    vPart->arcExists(l2, h));
#if ENABLE_DEEPER_STATS
                        ++op_s6;
#endif
                    }
                });

                if (!count_s[5]) {
                    return;
                }
                // Also consider high neighbors of l2 (excluding v)
                forEachHighNeighbor(l2, {v}, nullptr, [&](auto h) {
                    // Triangle (l - v - l2) +  extra edge l2 - h
                    operation2a(s5, h->getId(), v->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s5;
#endif
                });
            });
        }

        // s3: count 3path of low neighbors between v and l
        if (count_s[3]) {
            if (highAnchorsOnlyS3) {
                forEachHighNeighbor(v, {l}, nullptr, [&](auto n1) {
                    forEachHighNeighbor(l, {n1, v}, nullptr, [&](auto n2) {
                        operation2b(s3, n1->getId(), n2->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
                    });
                });

                forEachLowNeighbor(l, {v}, nullptr, [&](auto low) {
                    forEachHighNeighbor(low, {v, l}, nullptr, [&](auto x) {
                        operation2a(s3, v->getId(), x->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
                    });
                });
            } else {
                // Only consider v as vertex in the middle of the structure
                // since s3 is stored for high and low anchor vertices
                forEachNeighbor(v, {l}, [&](auto n1) {
                    forEachNeighbor(l, {n1, v}, [&](auto n2) {
                        operation2b(s3, n1->getId(), n2->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_s3;
#endif
                    });
                });
            }
        }

        // s4/s5: Use low neighbors and high neighbors of l (excluding v)
        if (count_s[4] || count_s[5]) {
            forEachLowNeighbor(l, {v}, nullptr, [&](auto l2) {
                forEachHighNeighbor(l, {v}, nullptr, [&](auto h) {
                    // Base pattern l connects to l2, h and v
                    if (count_s[4]) {
                        operation2a(s4, h->getId(), v->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_s4;
#endif
                    }
                    if (count_s[5] && vPart->arcExists(l2, h)) {
                        operation2a(s5, h->getId(), v->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_s5;
#endif
                    }
                });
            });
        }

        // If none of s4/s5/s6/s7 needed, skip
        if (!(count_s[4] || count_s[5] || count_s[6] || count_s[7])) {
            return;
        }

        // Pairwise high neighbors of v: (h1, h2) with the presence of v - l.
        forEachTwoHighNeighbors(v, {}, [&](auto h1, auto h2) {
            const int l_h1 = vPart->arcExists(l, h1);
            const int l_h2 = vPart->arcExists(l, h2);

            // s5 uses the sum: how many of h1/h2 connect to l
            if (count_s[5] && l_h1 + l_h2 > 0) {

                operation2b(s5, h1->getId(), h2->getId(), l_h1 + l_h2);
#if ENABLE_DEEPER_STATS
                ++op_s5;
#endif
            }

            // s6 uses the conjunction: both h1 and h2 connect to l
            if (count_s[6] && l_h1 && l_h2) {

                operation2b(s6, h1->getId(), h2->getId(), 1);
#if ENABLE_DEEPER_STATS
                ++op_s6;
#endif
            }
        });

        if (count_s[4] || count_s[5] || count_s[7]) {
            // Pairwise high neighbors of l (excluding v): (h1, h2)
            // Base pattern l connects to h1, h2 and v
            forEachTwoHighNeighbors(l, {v}, [&](auto h1, auto h2) {
                // s4: baseline count for pairwise high neighbors of l
                if (count_s[4]) {

                    operation2b(s4, h1->getId(), h2->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s4;
#endif
                }

                // s5: how many of (h1, h2) connect back to v
                const int v_h1 = vPart->arcExists(v, h1);
                const int v_h2 = vPart->arcExists(v, h2);
                if (count_s[5] && v_h1 + v_h2 > 0) {

                    operation2b(s5, h1->getId(), h2->getId(), v_h1 + v_h2);
#if ENABLE_DEEPER_STATS
                    ++op_s5;
#endif
                }
                // s7: triple v, h1, h2 connect to l
                operation3a(s7, v->getId(), h1->getId(), h2->getId(), 1);
#if ENABLE_DEEPER_STATS
                ++op_s7;
#endif
            });
        }
    });

    // After processing each low neighbor l, do pairwise high neighbors of v
    // globally
    // Base pattern v connects to h1 and h2
    if (count_s[2] || count_s[4] || count_s[7]) {

        forEachTwoHighNeighbors(v, {}, [&](auto h1, auto h2) {
            // s2: count pairwise high neighbors of v
            if (count_s[2]) {

                operation2b(s2, h1->getId(), h2->getId(), 1);
#if ENABLE_DEEPER_STATS
                ++op_s2;
#endif
            }
            // s4: weight by the low-degree of v (excluding removed)
            if (count_s[4]) {
                operation2b(s4, h1->getId(), h2->getId(),
                            vPart->getLowDegreeWithoutRemoved(v));
#if ENABLE_DEEPER_STATS
                ++op_s4;
#endif
            }
            // For triples among high neighbors of v: (h1, h2, h3) with h3 !=
            // h1,h2
            if (count_s[7]) {
                forEachHighNeighbor(v, {h1}, h2, [&](auto h3) {
                    operation3b(s7, h1->getId(), h2->getId(), h3->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_s7;
#endif
                });
            }
        });
    }
}