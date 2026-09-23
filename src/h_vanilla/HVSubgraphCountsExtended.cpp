#include "graph.incidencelist/incidencelistvertex.h"
#include "h_vanilla/HVSubgraphCounts.h"
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
COUNTER_TYPE HVSubgraphCounts::getNrClawsWithEdge(ILV *head, ILV *tail) {
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
COUNTER_TYPE HVSubgraphCounts::getNrTPathsWithEdge(ILV *head, ILV *tail) {
    bool headHigh{is_high(head)};
    bool tailHigh{is_high(tail)};
    COUNTER_TYPE ret{0};

    auto head_deg = graph->getUndirectedDegree(head);
    auto tail_deg = graph->getUndirectedDegree(tail);
    // 1. Edge is in center of path
    ret += (head_deg - 1) * (tail_deg - 1) - getNrTriangleWithEdge(head, tail);

    // 2. Path starts with edge + next vertex in path has low degree
    COUNTER_TYPE pathsStartingWithEdge = 0;
    pathsStartingWithEdge =
        StrucCount.getNrvLV(tail) - (headHigh ? 0 : head_deg - 1) +
        StrucCount.getNrvLV(head) - (tailHigh ? 0 : tail_deg - 1) -
        2 * StrucCount.getNruLv(head, tail);
    if (pathsStartingWithEdge > 0)
        ret += pathsStartingWithEdge;

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
COUNTER_TYPE HVSubgraphCounts::getNrTriangleWithEdge(ILV *u, ILV *v) {
    COUNTER_TYPE triangle{0};

    // Both are high!
    triangle += StrucCount.getNruLv(u, v);

    StrucCount.forLowerHighDegree(u, v, [&](auto ldv, auto hdv) {
        forEachHighNeighbor(ldv, {hdv}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            if (edge_exist(x, hdv))
                triangle++;
        });
    });

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
COUNTER_TYPE HVSubgraphCounts::getNrTriangleWithEdge_pretended(ILV *u, ILV *v,
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
COUNTER_TYPE HVSubgraphCounts::getNrTriangle_pretended(ILV *v, ILV *e1,
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
COUNTER_TYPE HVSubgraphCounts::getNrTriangle(ILV *v) {
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
COUNTER_TYPE HVSubgraphCounts::getNrPawsWithEdge(ILV *head, ILV *tail,
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
    ret += StrucCount.getNrcLV(head, tail);
    forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
        ++iterate_h;
#endif
        if (edge_exist(tail, h))
            ret += graph->getUndirectedDegree(h) - 2;
    });

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
COUNTER_TYPE HVSubgraphCounts::getNrfCyclesWithEdge(ILV *head, ILV *tail) {
    bool head_high = is_high(head);
    bool tail_high = is_high(tail);
    COUNTER_TYPE ret{0};

    // 1. Find two low in middle
    ret += StrucCount.getNruLLv(head, tail);

    auto findOneHighInMiddle = [&](auto v, auto other, auto count_all_high) {
        bool v_is_high = is_high(v);
        bool other_is_high = is_high(other);

        forEachHighNeighbor(v, {other}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            // 2. Find one high one low in middle
            auto uLv = StrucCount.getNruLv(h, other);
            ret += uLv ? (uLv - !v_is_high) : 0;

            // 3.(a) Find two high in middle if other is high degree
            if (count_all_high) {
                auto uHv = StrucCount.getNruHv(h, other);
                ret += uHv ? (uHv - v_is_high) : 0;
            }
        });
    };

    auto count_all_high_at_head =
        tail_high && ((head_high && (StrucCount.vPart->getHighDegree(head) <
                                     StrucCount.vPart->getHighDegree(tail))) ||
                      (!head_high));

    findOneHighInMiddle(head, tail, count_all_high_at_head);
    findOneHighInMiddle(tail, head, (!count_all_high_at_head) && head_high);

    // 3.(b) Find two high in middle if head and tail are low degree
    if (!head_high && !tail_high) {

        StrucCount.forLowerHighDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachHighNeighbor(ldv, {hdv}, nullptr, [&](auto n1) {
#if ENABLE_DEEPER_STATS
                ++iterate_neighbor;
#endif
                StrucCount.forLowerHighDegree(
                    n1, hdv, [&](auto ldv2, auto hdv2) {
                        forEachHighNeighbor(ldv2, {ldv, hdv2}, nullptr,
                                            [&](auto n2) {
#if ENABLE_DEEPER_STATS
                                                ++iterate_neighbor;
#endif
                                                if (edge_exist(hdv2, n2))
                                                    ret++;
                                            });
                    });
            });
        });
    } else {
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
COUNTER_TYPE HVSubgraphCounts::getNrDiamondsWithEdge(ILV *head, ILV *tail) {
    COUNTER_TYPE ret{0};

    // 1. Count all diamonds were head_tail is the chord
    if (is_high(head) && is_high(tail)) {
        double p =
            StrucCount.getNruLv(head, tail) + StrucCount.getNruHv(head, tail);
        if (p > 1) {
            COUNTER_TYPE pot = boost::math::binomial_coefficient<double>(p, 2.);
            ret += pot;
        }
    } else {
        StrucCount.forLowerDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachNeighbor(ldv, {hdv}, [&](auto x) {
                if (!edge_exist(x, hdv)) {
                    return;
                }
                forEachNeighbor(ldv, {hdv}, x,
                                [&](auto y) { ret += edge_exist(y, hdv); });
            });
        });
    }

    // 2. Count all diamonds were head_tail is part of the cycle
    // 2.(a) other two vertices have low degree
    ret += StrucCount.getNrpLL(head, tail);

    auto getAtLeastOneOtherIsHighDeg = [&](auto u, auto v,
                                           auto count_high_incident_to_chord) {
        forEachHighNeighbor(u, {v}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++iterate_h;
#endif
            // 2.(b) other vertex incident to the chord has low degree and the
            // fourth vertex has high degree
            ret += StrucCount.getNrcL(u, v, h);

            // 2.(c) other vertex incident to the chord has high degree
            if (count_high_incident_to_chord && edge_exist(h, v)) {
                if (is_high(u)) {
                    ret += StrucCount.getNruLv(u, h) +
                           StrucCount.getNruHv(u, h) - 1;
                } else {
                    forEachNeighbor(u, {v, h}, nullptr, [&](auto x) {
#if ENABLE_DEEPER_STATS
                        ++iterate_neighbor;
#endif
                        ret += edge_exist(h, x);
                    });
                }
            }
        });
    };

    bool headIsHigh = is_high(head);
    bool tailIsHigh = is_high(tail);
    bool headHasLowerHighDegree = StrucCount.vPart->getHighDegree(head) <
                                  StrucCount.vPart->getHighDegree(tail);
    bool headHasLowerUndirectedDegree =
        graph->getUndirectedDegree(head) < graph->getUndirectedDegree(tail);

    auto count_high_incident_to_chord_at_head =
        (headIsHigh && (!tailIsHigh || headHasLowerHighDegree)) ||
        (!tailIsHigh && headHasLowerUndirectedDegree);

    getAtLeastOneOtherIsHighDeg(head, tail, true);
    getAtLeastOneOtherIsHighDeg(tail, head, true);

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
COUNTER_TYPE HVSubgraphCounts::getNrFCliquesWithEdge(ILV *head, ILV *tail) {
    bool head_high = is_high(head);
    bool tail_high = is_high(tail);
    COUNTER_TYPE ret{0};
    int d1 = StrucCount.vPart->getLowDegree(head);
    int d2 = StrucCount.vPart->getLowDegree(tail);
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

    return ret;
}
