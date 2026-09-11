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
void HVStructureCounts::uHvArcChange(const Algora::Arc *a,
                                     IncOrDecDoubleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    bool head_high = is_high(head);
    bool tail_high = is_high(tail);

    if (head_high && tail_high) {
        forEachHighNeighbor(head, {tail}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++op_uHv;
#endif
            operation(uHv, h->getId(), tail->getId(), 1);
        });

        forEachHighNeighbor(tail, {head}, nullptr, [&](auto h) {
#if ENABLE_DEEPER_STATS
            ++op_uHv;
#endif
            operation(uHv, head->getId(), h->getId(), 1);
        });
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
 */
void HVStructureCounts::uHvVertexChange(ILV *v, IncOrDecDoubleKey *operation) {

    auto endPointOfRemoved = getLastRemovedIfAnyOtherThen(v);

    forEachHighNeighbor(
        v, {endPointOfRemoved}, nullptr, [&](auto highNeighbor1) {
            forEachHighNeighbor(v, {endPointOfRemoved}, highNeighbor1,
                                [&](auto highNeighbor2) {
#if ENABLE_DEEPER_STATS
                                    ++op_uHv;
#endif
                                    operation(uHv, highNeighbor1->getId(),
                                              highNeighbor2->getId(), 1);
                                });
        });

    if (is_high(v)) {
        forEachHighNeighbor(
            v, {endPointOfRemoved}, nullptr, [&](auto highNeighbor) {
                forEachHighNeighbor(highNeighbor, {v}, nullptr, [&](auto h) {

#if ENABLE_DEEPER_STATS
                    ++op_uHv;
#endif
                    operation(uHv, h->getId(), v->getId(), 1);
                });
            });

    } else {
        for (auto h = vPart->highDegVBegin(); h != vPart->highDegVEnd(); h++) {
            if (h->second != v) {
#if ENABLE_DEEPER_STATS
                ++op_uHv;
#endif
                operation(uHv, h->first, v->getId(), INT_MAX);
            }
        }
    }
}
