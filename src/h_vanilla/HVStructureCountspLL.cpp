#include "h_vanilla/HVStructureCounts.h"

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
void HVStructureCounts::pLLArcChange(const Algora::Arc *a,
                                     IncOrDecDoubleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_low{is_low(head)};
    bool tail_low{is_low(tail)};
    bool buildsTriangle;

    auto findPLLfromHighLow = [&](auto vLow, auto vOther) {
        forEachLowNeighbor(vLow, {vOther}, nullptr, [&](auto neighborLow) {
            buildsTriangle = edge_exist(neighborLow, vOther);

            forEachNeighbor(vLow, {vOther, neighborLow}, nullptr,
                            [&](auto neighborHigh) {
                                // Is (vLow,vOther,neighborLow) a triangle? +
                                // neighborHigh (from vLow) -> one pLL
                                if (buildsTriangle) {
#if ENABLE_DEEPER_STATS
                                    ++op_pLL;
#endif
                                    operation(pLL, vOther->getId(),
                                              neighborHigh->getId(), 1);
                                }
                                // Is (vLow,neighborLow,neighborHigh) a
                                // triangle? + vOther -> one pLL
                                if (edge_exist(neighborLow, neighborHigh)) {
#if ENABLE_DEEPER_STATS
                                    ++op_pLL;
#endif
                                    operation(pLL, vOther->getId(),
                                              neighborHigh->getId(), 1);
                                }
                            });

            if (!buildsTriangle)
                return;

            forEachNeighbor(
                neighborLow, {vLow, vOther}, nullptr, [&](auto neighborHigh) {
                    // Is (vLow,vOther,neighborLow) a triangle? + neighborHigh
                    // (from neighborLow) -> one pLL

                    operation(pLL, vOther->getId(), neighborHigh->getId(), 1);
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                });
        });
    };

    if (head_low) {
        findPLLfromHighLow(head, tail);
    }

    if (tail_low) {
        findPLLfromHighLow(tail, head);
    }

    if (head_low && tail_low) {

        auto findPLLfromLowLow = [&](auto low1, auto low2) {
            forEachNeighbor(low1, {low2}, nullptr, [&](auto h) {
                // Is (low1, low2, h) a triangle?
                if (!edge_exist(h, low2))
                    return;

                forEachNeighbor(low1, {low2, h}, nullptr, [&](auto h2) {
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
void HVStructureCounts::pLLVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                                        IncOrDecDoubleKey *operation2) {

    // Find pLL with v as non-anchor
    forEachLowNeighbor(v, {}, nullptr, [&](auto vLow) {
        forEachNeighbor(v, {vLow}, nullptr, [&](auto v1) {
            forEachNeighbor(v, {vLow}, v1, [&](auto v2) {
                // triangle v,v1,vLow? + v-v2
                if (edge_exist(v1, vLow)) {
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                    operation1(pLL, v1->getId(), v2->getId(), 1);
                }
                // triangle v,v2,vLow? + v-v1
                if (edge_exist(v2, vLow)) {
#if ENABLE_DEEPER_STATS
                    ++op_pLL;
#endif
                    operation1(pLL, v1->getId(), v2->getId(), 1);
                }
            });

            if (!edge_exist(vLow, v1))
                return;

            // triangle v,v1,vLow + vLow-v2?
            forEachNeighbor(vLow, {v, v1}, nullptr, [&](auto v2) {
#if ENABLE_DEEPER_STATS
                ++op_pLL;
#endif
                operation1(pLL, v1->getId(), v2->getId(), 1);
            });
        });
    });
}
