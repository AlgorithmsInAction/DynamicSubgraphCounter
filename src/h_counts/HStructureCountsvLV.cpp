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
void HStructureCounts::vLVVertexChange(ILV *v, IncOrDecSingleKey *operation1,
                                       IncOrDecSingleKey *operation2) {
    int deg_v = vPart->getDegreeWithoutRemoved(v);

    forEachHighNeighbor(v, {}, nullptr, [&](auto neighbor) {
#if ENABLE_DEEPER_STATS
        ++op_vLV;
#endif
        operation1(vLV, neighbor->getId(), deg_v - 1);
    });

    if (is_low(v)) {
        // If swap to Low just set all v anchors 0
        this->setZero(vLV, v);
    } else {
        forEachLowNeighbor(v, {}, nullptr, [&](auto neighbor) {
#if ENABLE_DEEPER_STATS
            ++op_vLV;
#endif
            operation2(vLV, v->getId(),
                       vPart->getDegreeWithoutRemoved(neighbor) - 1);
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
 * @param recompute_after_removed true if this function is called as part of a
 * recomputation after an edge has been removed
 */
void HStructureCounts::vLVArcChange(const Algora::Arc *a,
                                    IncOrDecSingleKey *operation,
                                    bool recompute_after_removed) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());

    int deg_h{0}, deg_t{0};

    if (recompute_after_removed) {
        deg_h = vPart->getDegreeWithoutRemoved(head);
        deg_t = vPart->getDegreeWithoutRemoved(tail);
    } else {
        deg_h = head->getUndirectedDegree();
        deg_t = tail->getUndirectedDegree();
    }

    auto head_low = is_low(head);
    auto tail_low = is_low(tail);

    if (head_low) {
        forEachHighNeighbor(head, {tail}, nullptr, [&](auto neighborX) {
#if ENABLE_DEEPER_STATS
            ++op_vLV;
#endif
            operation(vLV, neighborX->getId(), 1);
        });

        if (!tail_low) {
#if ENABLE_DEEPER_STATS
            ++op_vLV;
#endif
            operation(vLV, tail->getId(), deg_h - 1);
        }
    }

    if (tail_low) {
        forEachHighNeighbor(tail, {head}, nullptr, [&](auto neighborX) {
#if ENABLE_DEEPER_STATS
            ++op_vLV;
#endif
            operation(vLV, neighborX->getId(), 1);
        });

        if (!head_low) {
#if ENABLE_DEEPER_STATS
            ++op_vLV;
#endif
            operation(vLV, head->getId(), deg_t - 1);
        }
    }
}
