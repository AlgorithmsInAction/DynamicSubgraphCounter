#include "graph/graph_functional.h"
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
void HVStructureCounts::cLArcChange(const Algora::Arc *a,
                                    IncOrDecTripleKey *operation) {
    auto *head = CAST_ILV(a->getHead());
    auto *tail = CAST_ILV(a->getTail());
    bool head_low, tail_low;

    head_low = is_low(head);
    tail_low = is_low(tail);

    if (head_low) {
        forEachNeighbor(head, {tail}, [&](auto neighborX) {
            forEachNeighbor(head, {tail}, neighborX, [&](auto neighborY) {
#if ENABLE_DEEPER_STATS
                ++op_cL;
#endif
                operation(cL, neighborX->getId(), neighborY->getId(),
                          tail->getId(), 1);
            });
        });
    }

    if (tail_low) {
        forEachNeighbor(tail, {head}, [&](auto neighborX) {
            forEachNeighbor(tail, {head}, neighborX, [&](auto neighborY) {
#if ENABLE_DEEPER_STATS
                ++op_cL;
#endif
                operation(cL, neighborX->getId(), neighborY->getId(),
                          head->getId(), 1);
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
void HVStructureCounts::cLVertexChange(ILV *v, IncOrDecTripleKey *operation1,
                                       IncOrDecTripleKey *operation2) {

    forEachNeighbor(v, {}, nullptr, [&](auto neighborX) {
        forEachNeighbor(v, {}, neighborX, [&](auto neighborY) {
            forEachNeighbor(v, {neighborX}, neighborY, [&](auto neighborZ) {
#if ENABLE_DEEPER_STATS
                ++op_cL;
#endif
                operation1(cL, neighborX->getId(), neighborY->getId(),
                           neighborZ->getId(), 1);
            });
        });
    });
}
