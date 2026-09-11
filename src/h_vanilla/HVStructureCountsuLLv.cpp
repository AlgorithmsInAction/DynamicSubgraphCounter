#include "algorithm/valuecomputingalgorithm.h"
#include "graph/arc.h"
#include "graph/vertex.h"
#include "h_vanilla/HVStructureCounts.h"
#include <vector>

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
void HVStructureCounts::uLLvArcChange(const Algora::Arc *a,
                                      IncOrDecDoubleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_low = is_low(head);
    bool tail_low = is_low(tail);

    if (head_low) {
        forEachLowNeighbor(head, {tail}, nullptr, [&](auto lowNeighbor) {
            forEachNeighbor(
                lowNeighbor, {head, tail}, nullptr, [&](auto highNeighbor) {
#if ENABLE_DEEPER_STATS
                    ++op_uLLv;
#endif
                    operation(uLLv, tail->getId(), highNeighbor->getId(), 1);
                });
        });
    }

    if (tail_low) {

        forEachLowNeighbor(tail, {head}, nullptr, [&](auto lowNeighbor) {
            forEachNeighbor(
                lowNeighbor, {head, tail}, nullptr, [&](auto highNeighbor) {
#if ENABLE_DEEPER_STATS
                    ++op_uLLv;
#endif
                    operation(uLLv, head->getId(), highNeighbor->getId(), 1);
                });
        });
    }

    if (head_low && tail_low) {
        forEachNeighbor(head, {tail}, nullptr, [&](auto highNeighborHead) {
            forEachNeighbor(tail, {head, highNeighborHead}, nullptr,
                            [&](auto highNeighborTail) {
#if ENABLE_DEEPER_STATS
                                ++op_uLLv;
#endif
                                operation(uLLv, highNeighborHead->getId(),
                                          highNeighborTail->getId(), 1);
                            });
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
 * @param operation2 function that either increments or decrements the a given
 * hash map. This should increment if the vertex changes to high degree and
 * decrement otherwise
 */
void HVStructureCounts::uLLvVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                                         IncOrDecDoubleKey *operation2) {

    forEachLowNeighbor(v, {}, nullptr, [&](auto low_v) {
        forEachNeighbor(low_v, {v}, nullptr, [&](auto h) {
            forEachNeighbor(v, {h, low_v}, nullptr, [&](auto h2) {
#if ENABLE_DEEPER_STATS
                ++op_uLLv;
#endif
                // h2 - v - low - h
                operation1(uLLv, h->getId(), h2->getId(), 1);
            });
        });
    });
}
