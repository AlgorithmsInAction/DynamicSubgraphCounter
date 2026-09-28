#include "graph.visitor/arcvisitor.h"
#include "graph/arc.h"
#include "h_counts/HSubgraphCounts.h"

/**
 * @brief Gives the number of Triangles that contain the edge head-tail and the
 * vertex s. It is assumed, that the vertices head and s are always connected by
 * an edge. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sTriangleCount(ILV *head, ILV *tail, ILV *s) {
    COUNTER_TYPE increment{0};
    if (s != tail) {
        if (edge_exist(tail, s)) {
            increment++;
        }
    } else if (s == tail) {
        increment += this->getNrTriangleWithEdge(s, head);
    }
    return increment;
}

/**
 * @brief Gives the number of Three-Paths that contain the edge head-tail and
 * the vertex s. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sTPathCount(ILV *head, ILV *tail, ILV *s, bool,
                                          bool) {
    COUNTER_TYPE increment{0};
    if (head == s || tail == s) {

        increment += getNrTPathsWithEdge(head, tail);
    } else {

        bool Esh = edge_exist(s, head);
        bool Est = edge_exist(s, tail);

        if (Esh && !Est) {

            increment += s->getUndirectedDegree() - 1 +
                         tail->getUndirectedDegree() - 1;
        }
        if (!Esh && Est) {

            increment += s->getUndirectedDegree() - 1 +
                         head->getUndirectedDegree() - 1;
        }
        if (Esh && Est) {

            increment += 2 * (s->getUndirectedDegree() - 2) +
                         tail->getUndirectedDegree() - 2 +
                         head->getUndirectedDegree() - 2;
        }
    }

    return increment;
}

/**
 * @brief Gives the number of Claws that contain the edge head-tail and the
 * vertex s. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @param deg_h Degree of the vertex head
 * @param deg_t Degree of the vertex tail
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sClawCount(ILV *head, ILV *tail, ILV *s,
                                         COUNTER_TYPE deg_h,
                                         COUNTER_TYPE deg_t) {
    COUNTER_TYPE increment{0};

    if (head == s || tail == s) {
        increment += getNrClawsWithEdge(head, tail);
    } else {
        if (deg_h >= 1 && edge_exist(s, head)) {

            increment += deg_h - 1;
        }
        if (deg_t >= 1 && edge_exist(s, tail)) {

            increment += deg_t - 1;
        }
    }
    return increment;
}

/**
 * @brief Gives the number of Paws that contain the edge head-tail and the
 * vertex s. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @param arc_removed If the arc head-tail has been removed or inserted
 * @param tHead Number of triangles containing the vertex head
 * @param tTail Number of triangles containing the vertex tail
 * @param tHeadTail Number of triangles containing the edge head-tail.
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sPawCount(ILV *head, ILV *tail, ILV *s, bool,
                                        bool, bool arc_removed,
                                        COUNTER_TYPE tHead, COUNTER_TYPE tTail,
                                        COUNTER_TYPE tHeadTail) {
    COUNTER_TYPE increment{0};
    if (head == s || tail == s) {
        increment += getNrPawsWithEdge(head, tail, tHead, tTail, tHeadTail);
    } else {
        bool Est{edge_exist(s, tail)};
        bool Esh{edge_exist(s, head)};

        if (Est) {
            if (arc_removed) {
                increment +=
                    getNrTriangleWithEdge_pretended(s, tail, head, tail);
                increment += tHeadTail;
            } else {
                increment += getNrTriangleWithEdge(s, tail);
                increment += tHeadTail;
            }
            if (Esh)
                increment -= 2;
        }
        if (Esh) {
            if (arc_removed) {
                increment +=
                    getNrTriangleWithEdge_pretended(s, head, head, tail);
                increment += tHeadTail;
            } else {
                increment += getNrTriangleWithEdge(s, head);
                increment += tHeadTail;
            }
            if (Est)
                increment -= 2;
        }
        if (Esh && Est) {
            increment += head->getUndirectedDegree() - 2 +
                         tail->getUndirectedDegree() - 2 +
                         s->getUndirectedDegree() - 2;
        }
    }
    return increment;
}

/**
 * @brief Gives the number of Four-Cycles that contain the edge head-tail and
 * the vertex s. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @param headHigh if the vertex head is classified as high degree
 * @param tailHigh if the vertex tail is classified as low degree
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sFCycleCount(ILV *head, ILV *tail, ILV *s,
                                           bool headHigh, bool tailHigh) {
    COUNTER_TYPE increment{0};
    if (s == head || s == tail) {

        increment += getNrfCyclesWithEdge(head, tail);
    }

    else {
        bool Esh = edge_exist(s, head);
        bool Est = edge_exist(s, tail);
        bool sHigh{is_high(s)};

        if (!Esh && !Est)
            return 0;

        if (Esh) {
            if (sHigh && tailHigh)
                increment += StrucCount.getNruLv(s, tail) +
                             StrucCount.getNruHv(s, tail) - 1;
            else if (tailHigh) {
                for (auto const &a : s->getEdges()) {
                    auto x = CAST_ILV(a->getOther(s));
                    if (x == head || x == tail)
                        continue;
                    if (edge_exist(x, tail))
                        increment++;
                }
            }
        }
        if (Est) {
            if (sHigh && headHigh)
                increment += StrucCount.getNruLv(s, head) +
                             StrucCount.getNruHv(s, head) - 1;
            else if (headHigh) {
                for (auto const &a : s->getEdges()) {
                    auto x = CAST_ILV(a->getOther(s));
                    if (x == head || x == tail)
                        continue;
                    if (edge_exist(x, head))
                        increment++;
                }
            }
        }
    }

    return increment;
}

/**
 * @brief Gives the number of Diamonds that contain the edge head-tail and the
 * vertex s. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sDiamondCount(ILV *head, ILV *tail, ILV *s, bool,
                                            bool) {
    COUNTER_TYPE increment{0};

    if (s == head || s == tail) {

        increment += getNrDiamondsWithEdge(head, tail);
    } else {
        bool Esh = edge_exist(s, head);
        bool Est = edge_exist(s, tail);
        bool arc_removed{!edge_exist(head, tail)};
        if (Esh && Est) {

            increment += this->getNrTriangleWithEdge(head, tail) - 1;
            if (arc_removed) {
                increment +=
                    this->getNrTriangleWithEdge_pretended(s, tail, head, tail) -
                    1;
                increment +=
                    this->getNrTriangleWithEdge_pretended(s, head, head, tail) -
                    1;
            } else {
                increment += this->getNrTriangleWithEdge(s, tail) - 1;
                increment += this->getNrTriangleWithEdge(s, head) - 1;
            }
        }
    }
    return increment;
}

/**
 * @brief Gives the number of Four-Cliques that contain the edge head-tail and
 * the vertex s. All three given vertices must be distinct.
 *
 * @param head vertex one of the edge
 * @param tail vertex two of the edge
 * @param s vertex for which the change in s-count should be updated
 * @param headHigh if the vertex head is classified as high degree
 * @param tailHigh if the vertex tail is classified as low degree
 * @return int
 */
COUNTER_TYPE HSubgraphCounts::sFCliqueCount(ILV *head, ILV *tail, ILV *s,
                                            bool headHigh, bool tailHigh) {
    COUNTER_TYPE increment{0};
    if (s == head || s == tail) {

        increment += getNrFCliquesWithEdge(head, tail);
    }

    else {
        if (edge_exist(head, s) && edge_exist(tail, s)) {
            bool sHigh{is_high(s)};

            if (headHigh && tailHigh && sHigh) {
                increment += StrucCount.getNrcL(head, tail, s);
                forEachHighNeighbor(s, {head, tail}, nullptr, [&](auto h) {
                    if (edge_exist(h, head) && edge_exist(h, tail))
                        increment++;
                });
            } else if (!headHigh) {
                for (auto const &a : head->getEdges()) {
                    auto x = CAST_ILV(a->getOther(head));
                    if (x == tail || x == s || !edge_exist(x, tail) ||
                        !edge_exist(x, s))
                        continue;
                    increment++;
                }

            } else if (!tailHigh) {
                for (auto const &a : tail->getEdges()) {
                    auto x = CAST_ILV(a->getOther(tail));
                    if (x == head || x == s || !edge_exist(x, head) ||
                        !edge_exist(x, s))
                        continue;
                    increment++;
                }

            } else if (!sHigh) {
                for (auto const &a : s->getEdges()) {
                    auto x = CAST_ILV(a->getOther(s));
                    if (x == head || x == tail || !edge_exist(x, tail) ||
                        !edge_exist(x, head))
                        continue;
                    increment++;
                }
            }
        }
    }
    return increment;
}
