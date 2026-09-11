#include "h_counts/HStructureCounts.h"

/**
 * @brief updates the auxiliary count by calculating the number the count
 * changes by when inserting/removing the given arc. This is then given to a
 * function that either increases or decreases the count by that number. This
 * depends on if the arc is added or deleted.
 *
 * @param a arc that is added/deleted
 * @param operation function that either increments or decrements the a given
 * hash map. This should decrement if the arc is removed and increment if the
 * arc in inserted
 */
void HStructureCounts::pLLArcChange(const Algora::Arc *a,
                                    IncOrDecDoubleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_low{is_low(head)};
    bool tail_low{is_low(tail)};
    bool buildsTriangle;

    auto findPLLfromHighLow = [&](auto vLow, auto vHigh) {
        forEachLowNeighbor(vLow, {vHigh}, nullptr, [&](auto neighborLow) {
            buildsTriangle = edge_exist(neighborLow, vHigh);

            forEachHighNeighbor(vLow, {vHigh, neighborLow}, nullptr,
                                [&](auto neighborHigh) {
                                    // Is (vLow,vHigh,neighborLow) a triangle? +
                                    // neighborHigh (from vLow) -> one pLL
                                    if (buildsTriangle) {
#if ENABLE_DEEPER_STATS
                                        ++op_pLL;
#endif
                                        operation(pLL, vHigh->getId(),
                                                  neighborHigh->getId(), 1);
                                    }
                                    // Is (vLow,neighborLow,neighborHigh) a
                                    // triangle? + vHigh -> one pLL
                                    if (edge_exist(neighborLow, neighborHigh)) {
#if ENABLE_DEEPER_STATS
                                        ++op_pLL;
#endif
                                        operation(pLL, vHigh->getId(),
                                                  neighborHigh->getId(), 1);
                                    }
                                });

            if (!buildsTriangle)
                return;

            forEachHighNeighbor(
                neighborLow, {vLow, vHigh}, nullptr, [&](auto neighborHigh) {
                    // Is (vLow,vHigh,neighborLow) a triangle? + neighborHigh
                    // (from neighborLow) -> one pLL

                    operation(pLL, vHigh->getId(), neighborHigh->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                });
        });
    };

    if (head_low && !tail_low) {
        findPLLfromHighLow(head, tail);
    } else if (tail_low && !head_low) {
        findPLLfromHighLow(tail, head);
    } else if (head_low && tail_low) {

        auto findPLLfromLowLow = [&](auto low1, auto low2) {
            forEachHighNeighbor(low1, {low2}, nullptr, [&](auto h) {
                // Is (low1, low2, h) a triangle?
                if (!edge_exist(h, low2))
                    return;

                forEachHighNeighbor(low1, {h}, nullptr, [&](auto h2) {
                    // (low1, low2, h) is triangle + h2 (from low1) -> one pLL
                    operation(pLL, h->getId(), h2->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                });
            });
        };

        findPLLfromLowLow(head, tail);
        findPLLfromLowLow(tail, head);
    }
}

/**
 * @brief updates the auxiliary count by calculating the number the count should
 * change by when the given vertex changes from one epsilon partition to the
 * other. This is then given to a function that either increases or decreases
 * the count by that number. This depends on if the vertex changes from low to
 * high or the other way around
 *
 * @param v vertex that changes partition
 * @param operation1 function that either increments or decrements the a given
 * hash map. This should decrement if the vertex changes to high degree and
 * increment otherwise
 * @param operation2 function that either increments or decrements the a given
 * hash map. This should increment if the vertex changes to high degree and
 * decrement otherwise
 */
void HStructureCounts::pLLVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                                       IncOrDecDoubleKey *operation2) {

    // Find pLL with v as non-anchor
    forEachHighNeighbor(v, {}, nullptr, [&](auto vHigh1) {
        forEachHighNeighbor(v, {}, vHigh1, [&](auto vHigh2) {
            forEachLowNeighbor(v, {vHigh1, vHigh2}, nullptr, [&](auto vLow) {
                // triangle v,vHigh1,vLow? + v-vHigh2
                if (edge_exist(vHigh1, vLow)) {
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                    operation1(pLL, vHigh1->getId(), vHigh2->getId(), 1);
                }
                // triangle v,vHigh2,vLow? + v-vHigh1
                if (edge_exist(vHigh2, vLow)) {
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                    operation1(pLL, vHigh1->getId(), vHigh2->getId(), 1);
                }
            });
        });

        forEachLowNeighbor(v, {vHigh1}, nullptr, [&](auto vLow) {
            if (!edge_exist(vLow, vHigh1))
                return;

            // triangle v,vHigh1,vLow + vLow-vHigh2?
            forEachHighNeighbor(vLow, {v, vHigh1}, nullptr, [&](auto vHigh2) {
#if ENABLE_DEEPER_STATS
                ++op_pLL;
#endif
                operation1(pLL, vHigh1->getId(), vHigh2->getId(), 1);
            });
        });
    });
    auto v_low_deg = vPart->getLowDegreeWithoutRemoved(v);
    auto max_low_deg = vPart->getTheoreticalMaxLowDegree();
    bool iterate_H_toDelete{is_low(v) &&
                            vPart->get_num_high_deg() <
                                v_low_deg * max_low_deg * max_low_deg};

    // Find pLL with v as anchor
    // If we swap to low, we need to delete all structures with v as anchor
    // We can do this by iterating h or finding the actual structures -> choose
    // faster
    if (iterate_H_toDelete) {
#if ENABLE_DEEPER_STATS
        op_pLL += vPart->get_num_high_deg();
#endif
        setZero(pLL, v);
    } else {
        forEachLowNeighbor(v, {}, nullptr, [&](auto vLow1) {
            forEachLowNeighbor(v, {}, vLow1, [&](auto vLow2) {
                if (!edge_exist(vLow1, vLow2))
                    return;
                forEachHighNeighbor(
                    vLow1, {v, vLow2}, nullptr, [&](auto vHigh) {
                        // triangle v,vLow1,vLow2 + vLow1-vHigh
                        operation2(pLL, v->getId(), vHigh->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_pLL;
#endif
                    });

                forEachHighNeighbor(
                    vLow2, {v, vLow1}, nullptr, [&](auto vHigh) {
                        // triangle v,vLow1,vLow2 + vLow2-vHigh
                        operation2(pLL, v->getId(), vHigh->getId(), 1);
#if ENABLE_DEEPER_STATS
                        ++op_pLL;
#endif
                    });
            });

            forEachHighNeighbor(vLow1, {v}, nullptr, [&](auto vHigh) {
                forEachLowNeighbor(vLow1, {v, vHigh}, nullptr, [&](auto vLow2) {
                    if (!edge_exist(vHigh, vLow2))
                        return;
                    // triangle vHigh,vLow1,vLow2 + v-vLow1
                    operation2(pLL, vHigh->getId(), v->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                });
            });
        });
    }
}
