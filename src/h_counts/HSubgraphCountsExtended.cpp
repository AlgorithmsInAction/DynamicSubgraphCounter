#include "graph.incidencelist/incidencelistvertex.h"
#include "h_counts/HSubgraphCounts.h"
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <cmath>
#include <iostream>

/**
 * @brief Count the number of Claws that contain the the edge that is given by
 * the two vertices. The vertices can be given in any order.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrClawsWithEdge(ILV *head, ILV *tail) {
    COUNTER_TYPE deg_h = graph->getUndirectedDegree(head) - 1;
    COUNTER_TYPE deg_t = graph->getUndirectedDegree(tail) - 1;

    COUNTER_TYPE ret{0};
    if (deg_t >= 2) {
        ret += 0.5 * deg_t * (deg_t - 1);
    }

    if (deg_h >= 2) {
        ret += 0.5 * deg_h * (deg_h - 1);
    }
    return ret;
}

/**
 * @brief Count the number of Three-Paths that contain the the edge that is
 * given by the two vertices. The vertices can be given in any order.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrTPathsWithEdge(ILV *head, ILV *tail) {
    bool headHigh{is_high(head)};
    bool tailHigh{is_high(tail)};
    COUNTER_TYPE ret{0};
    // 1. Edge is in center of path
    ret += (graph->getUndirectedDegree(head) - 1) *
               (graph->getUndirectedDegree(tail) - 1) -
           getNrTriangleWithEdge(head, tail);

    // 2. Path starts with edge + next vertex in path has low degree
    COUNTER_TYPE pathsStartingWithEdge = 0;

    auto updateIfOneIsLow = [&](auto low, auto other) {
        bool other_is_high{is_high(other)};

        if (other_is_high) {
            pathsStartingWithEdge = StrucCount.getNrvLV(other) -
                                    (graph->getUndirectedDegree(low) - 1);
        }
        StrucCount.forEachLowNeighbor(low, {other}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
            ++iterate_neighbor;
#endif
            if (edge_exist(x, other)) {
                if (other_is_high) {
                    --pathsStartingWithEdge;
                }
                ret += (graph->getUndirectedDegree(x) - 2);
            } else {
                ret += graph->getUndirectedDegree(x) - 1;
            }
        });
    };

    if (tailHigh && headHigh) {
        pathsStartingWithEdge = StrucCount.getNrvLV(tail) +
                                StrucCount.getNrvLV(head) -
                                2 * StrucCount.getNruLv(head, tail);
    } else {
        if (!headHigh) {
            updateIfOneIsLow(head, tail);
        }
        if (!tailHigh) {
            updateIfOneIsLow(tail, head);
        }
    }

    // 3. Path starts with edge + next vertex in path has high degree
    forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
        ++iterate_h;
#endif
        bool h_tail = edge_exist(h, tail);
        if (h_tail) {
            COUNTER_TYPE tmp = 2 * (graph->getUndirectedDegree(h) - 2);
            if (tmp > 0)
                ret += tmp;
        } else {
            COUNTER_TYPE tmp = graph->getUndirectedDegree(h) - 1;
            if (tmp > 0)
                ret += tmp;
        }
    });

    forEachHighNeighbor(tail, {head}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
        ++iterate_h;
#endif
        bool h_head = edge_exist(h, head);
        if (!h_head) {
            COUNTER_TYPE tmp = graph->getUndirectedDegree(h) - 1;
            if (tmp > 0)
                ret += tmp;
        }
    });

    if (pathsStartingWithEdge > 0)
        ret += pathsStartingWithEdge;

    return ret;
}

/**
 * @brief Count the number of Triangles that contain the the edge that is given
 * by the two vertices. The vertices can be given in any order.
 *
 * @param u vertex one of the edge
 * @param v vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrTriangleWithEdge(ILV *u, ILV *v) {
    COUNTER_TYPE triangle{0};

    if (use_aux_for_t && is_high(u) && is_high(v)) {
        // Both are high!
        triangle += StrucCount.getNruLv(u, v);

        if (use_all_aux) {
            triangle += StrucCount.getNruHv(u, v);
        } else {
            StrucCount.forLowerHighDegree(u, v, [&](auto ldv, auto hdv) {
                forEachHighNeighbor(ldv, {hdv}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                    ++iterate_h;
#endif
                    if (edge_exist(x, hdv))
                        triangle++;
                });
            });
        }
    } else {
        StrucCount.forLowerDegree(u, v, [&](auto ldv, auto hdv) {
            forEachNeighbor(ldv, {hdv}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (edge_exist(x, hdv))
                    triangle++;
            });
        });
    }
    return triangle;
}

/**
 * @brief Count the number of Triangles that contain the the edge that is given
 * by the two vertices. It is counted as if the edge connecting e1 to e2 also
 * exists. e1-e2 must not exist, but the Auxiliary Counts must also be counted
 * as if the edge exists. The vertices can be given in any order.
 *
 * @param u vertex one of the edge
 * @param v vertex two of the edge
 * @param e1 vertex one of the edge that should exist
 * @param e2 vertex two of the edge that should exist
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrTriangleWithEdge_pretended(ILV *u, ILV *v,
                                                              ILV *e1,
                                                              ILV *e2) {
    COUNTER_TYPE triangle{getNrTriangleWithEdge(u, v)};

    if (is_high(u) && is_high(v))
        return triangle;

    if (u == e1 && edge_exist(e2, v))
        triangle++;
    else if (u == e2 && edge_exist(e1, v))
        triangle++;
    else if (v == e1 && edge_exist(e2, u))
        triangle++;
    else if (v == e2 && edge_exist(e1, u))
        triangle++;

    return triangle;
}
/**
 * @brief Count the number of Triangles that contain the the vertex v.
 * It is counted as if the edge connecting e1 to e2 also exists. e1-e2 must not
 * exist, but the Auxiliary Counts must also be counted as if the edge exists.
 * The vertices e1 and e2 can be given in any order.
 *
 * @param v vertex two of the edge
 * @param e1 vertex one of the edge that should exist
 * @param e2 vertex two of the edge that should exist
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrTriangle_pretended(ILV *v, ILV *e1,
                                                      ILV *e2) {
    /* Acts as if the given edge is removed but all Aux-Counts affiliated with
     * the edge are not updated yet */
    if (is_high(v)) {
        return StrucCount.getNrt(v);
    } else {
        COUNTER_TYPE ret{0};
        forEachNeighbor(v, {}, [&](auto x) {
#if ENABLE_DEEPER_STATS
            ++iterate_neighbor;
#endif
            if ((x == e1 && v == e2) || (x == e2 && v == e1))
                return;

            if ((e1 == v && edge_exist(e2, x)) ||
                (e2 == v && edge_exist(e1, x)))
                ret++;
            forEachNeighbor(v, {}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if ((y == e1 && v == e2) || (y == e2 && v == e1))
                    return;

                if (edge_exist(x, y))
                    ret++;
            });
        });

        return ret;
    }
}

/**
 * @brief Count the number of Triangles that contain the the vertex v.
 *
 * @param v vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrTriangle(ILV *v) {
    if (is_high(v)) {
        return StrucCount.getNrt(v);
    } else {
        COUNTER_TYPE ret{0};
        forEachNeighbor(v, {}, [&](auto x) {
#if ENABLE_DEEPER_STATS
            ++iterate_neighbor;
#endif
            forEachNeighbor(v, {}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (edge_exist(x, y))
                    ret++;
            });
        });

        return ret;
    }
}

/**
 * @brief Count the number of Paws that contain the the edge that is given
 * by the two vertices. The vertices can be given in any order.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param tHead Nr of Triangles containing the vertex head
 * @param tTail Nr of Triangles containing the vertex tail
 * @param tHeadTail Nr of Triangles containing the edge head-tail
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrPawsWithEdge(ILV *head, ILV *tail,
                                                COUNTER_TYPE tHead,
                                                COUNTER_TYPE tTail,
                                                COUNTER_TYPE tHeadTail) {
    COUNTER_TYPE ret{0};
    COUNTER_TYPE deg_head = graph->getUndirectedDegree(head);
    COUNTER_TYPE deg_tail = graph->getUndirectedDegree(tail);

    // 1. edge is the arm of the paw
    ret += tHead + tTail - 2 * tHeadTail;

    // 2. edge is in the triangle and adjacent to the arm
    ret += tHeadTail * (deg_head - 2 + deg_tail - 2);

    // 3. edge is in the triangle and NOT adjacent to the arm
    if (is_high(head) && is_high(tail)) {
        ret += StrucCount.getNrcLV(head, tail);
        forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            if (edge_exist(tail, h))
                ret += graph->getUndirectedDegree(h) - 2;
        });
    } else {
        StrucCount.forLowerDegree(head, tail, [&](auto low, auto any) {
            StrucCount.forEachNeighbor(low, {any}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (edge_exist(x, any)) {
                    ret += graph->getUndirectedDegree(x) - 2;
                }
            });
        });
    }

    return ret;
}

/**
 * @brief Count the number of Four-Cycles that contain the the edge that is
 * given by the two vertices. The vertices can be given in any order.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrfCyclesWithEdge(ILV *head, ILV *tail) {
    bool head_high = is_high(head);
    bool tail_high = is_high(tail);
    COUNTER_TYPE ret{0};

    auto updateOneLow = [&](auto low, auto high) {
        forEachNeighbor(low, {high}, [&](auto x) {
#if ENABLE_DEEPER_STATS
            ++iterate_neighbor;
#endif
            if (is_low(x)) {
                forEachNeighbor(x, {low, high}, [&](auto y) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    if (edge_exist(y, high))
                        ret++;
                });
            } else {
                ret += StrucCount.getNruLv(x, high) - 1 +
                       StrucCount.getNruHv(x, high);
            }
        });
    };

    if (head_high && tail_high) {
        ret += StrucCount.getNruLLv(head, tail);
        auto findAtLeastOneHighInMiddle = [&](auto v, auto other,
                                              auto count_all_high) {
            forEachHighNeighbor(v, {other}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
                ++iterate_h;
#endif
                ret += StrucCount.getNruLv(h, other);
                // Count al high for the one with lower high degree
                if (count_all_high) {
                    ret += StrucCount.getNruHv(h, other) - 1;
                }
            });
        };

        auto head_has_lower_H_deg = StrucCount.vPart->getHighDegree(head) <
                                    StrucCount.vPart->getHighDegree(head);

        findAtLeastOneHighInMiddle(head, tail, head_has_lower_H_deg);
        findAtLeastOneHighInMiddle(tail, head, !head_has_lower_H_deg);
    } else if (head_high && !tail_high) {
        updateOneLow(tail, head);
    } else if (!head_high && tail_high) {
        updateOneLow(head, tail);
    } else {
        // All are low -> just iterate, but always choose lower degree to
        // iterate

        StrucCount.forLowerDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachNeighbor(ldv, {hdv}, nullptr, [&](auto n1) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                StrucCount.forLowerDegree(n1, hdv, [&](auto ldv2, auto hdv2) {
                    forEachNeighbor(ldv2, {ldv, hdv2}, nullptr, [&](auto n2) {
#if ENABLE_DEEPER_STATS
                        ++iterate_neighbor;
#endif
                        if (edge_exist(hdv2, n2))
                            ret++;
                    });
                });
            });
        });
    }
    return ret;
}

/**
 * @brief Count the number of Diamonds that contain the the edge that is
 * given by the two vertices. The vertices can be given in any order.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrDiamondsWithEdge(ILV *head, ILV *tail) {
    COUNTER_TYPE ret{0};

    bool head_high = is_high(head);
    bool tail_high = is_high(tail);

    auto updateOneLow = [&](auto low, auto high) {
        forEachNeighbor(low, {high}, [&](auto x) {
#if ENABLE_DEEPER_STATS
            ++iterate_neighbor;
#endif
            bool x_high{edge_exist(x, high)};

            // Count all diamond were low is not adjacent to the chord, x_high
            // is chord!
            if (x_high) {
                if (is_low(x)) {
                    forEachNeighbor(x, {low, high}, [&](auto y) {
#if ENABLE_DEEPER_STATS
                        ++iterate_neighbor;
#endif
                        ret += edge_exist(y, high);
                    });
                } else {
                    ret += StrucCount.getNruLv(high, x) - 1;
                    ret += StrucCount.getNruHv(high, x);
                }
            }

            forEachNeighbor(low, {high}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                // Count all diamond were low is adjacent to the chord
                auto y_high = edge_exist(y, high);
                auto x_y = edge_exist(y, x);
                ret += (x_high && y_high) + (x_high && x_y) + (y_high && x_y);
            });
        });
    };

    if (head_high && tail_high) {
        double p =
            StrucCount.getNruLv(head, tail) + StrucCount.getNruHv(head, tail);
        if (p > 1) {
            COUNTER_TYPE pot = boost::math::binomial_coefficient<double>(p, 2.);
            ret += pot;
        }
        ret += StrucCount.getNrpLL(head, tail);

        forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            ret += StrucCount.getNrcL(head, tail, h);

            if (edge_exist(h, tail)) {
                ret +=
                    StrucCount.getNruLv(head, h) + StrucCount.getNruLv(tail, h);
                ret += StrucCount.getNruHv(head, h) - 1 +
                       StrucCount.getNruHv(tail, h) - 1;
            }
        });

        forEachHighNeighbor(tail, {head}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            ret += StrucCount.getNrcL(head, tail, h);
        });
    } else if (!head_high && tail_high) {
        updateOneLow(head, tail);
    } else if (!tail_high && head_high) {
        updateOneLow(tail, head);

    } else {
        // All dow degree, so we just iterate but we always iterate the lower
        // degree vertex!

        StrucCount.forLowerDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachNeighbor(ldv, {hdv}, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                // Check for ldv adj to three vertices in the diamond
                forEachNeighbor(ldv, {hdv}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    // Now we have a claw: ldv is adjacent to hdv, x and y
                    auto y_hdv = edge_exist(y, hdv);
                    auto x_hdv = edge_exist(x, hdv);
                    auto x_y = edge_exist(x, y);

                    ret += (y_hdv && x_hdv) + (x_y && x_hdv) + (x_y && y_hdv);
                });

                auto b_a = edge_exist(x, hdv);

                if (!edge_exist(x, hdv)) {
                    return;
                }
                // Check for ldv adj to two vertices in the diamond
                // x - hdv is the chord now!
                StrucCount.forLowerDegree(x, hdv, [&](auto a, auto b) {
                    // Note we have a triangle a - b - ldv
                    forEachNeighbor(a, {b, ldv}, [&](auto c) {
#if ENABLE_DEEPER_STATS
                        ++iterate_neighbor;
#endif
                        // Now we have triangle a - b - ldv and and edge a - c
                        // For the diamond we also need c - b
                        ret += edge_exist(c, b);
                    });
                });
            });
        });
    }

    return ret;
}

/**
 * @brief Count the number of Four-Cliques that contain the the edge that is
 * given by the two vertices. The vertices can be given in any order.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::getNrFCliquesWithEdge(ILV *head, ILV *tail) {
    bool head_high = is_high(head);
    bool tail_high = is_high(tail);
    COUNTER_TYPE ret{0};
    int d1 = head->getUndirectedDegree();
    int d2 = tail->getUndirectedDegree();
    auto m = graph->getNumArcs(1);

    if (d1 * d1 > m && d2 * d2 > m) {
        // Need to run over all edges for O(m) runtime
        graph->mapEdges([&](auto edge) {
            auto x = edge->getHead();
            auto y = edge->getTail();
            if (x == head || x == tail || y == head || y == tail) {
                return;
            }

            if (edge_exist(x, head) && edge_exist(x, tail) &&
                edge_exist(y, head) && edge_exist(y, tail)) {
                ++ret;
            }
        });

        return ret;
    }

    if (use_all_aux && head_high && tail_high) {

        // 1. The other two vertices are low
        StrucCount.forLowerLowDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachLowNeighbor(ldv, {hdv}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                if (!edge_exist(x, hdv))
                    return;
                forEachLowNeighbor(ldv, {hdv}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
                    ++iterate_neighbor;
#endif
                    if (edge_exist(y, hdv) && edge_exist(y, x))
                        ret++;
                });
            });
        });

        StrucCount.forLowerHighDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachHighNeighbor(ldv, {hdv}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
                ++iterate_h;
#endif
                if (!edge_exist(h, hdv))
                    return;
                // 2. One other is high and one is low
                ret += StrucCount.getNrcL(ldv, hdv, h);

                // 3. All are high
                forEachHighNeighbor(ldv, {hdv}, h, [&](auto h2) {
#if ENABLE_DEEPER_STATS
                    ++iterate_h;
#endif
                    if (edge_exist(h, h2) && edge_exist(h2, hdv))
                        ret++;
                });
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
                        ret++;
                });
            });
        });
    }

    return ret;
}
