#include "graph/vertexpair.h"
#include "h_counts/HStructureCounts.h"
#include <algorithm>
#include <utility>

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
void HStructureCounts::uLvArcChange(const Algora::Arc *a,
                                    IncOrDecDoubleKey *operation) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_low;
    bool tail_low;

    head_low = is_low(head);
    tail_low = is_low(tail);

    if (head_low && !tail_low) {
        forEachHighNeighbor(head, {tail}, nullptr, [&](auto neighborX) {
#if ENABLE_DEEPER_STATS
            ++op_uLv;
#endif
            operation(uLv, tail->getId(), neighborX->getId(), 1);
        });
    }

    if (tail_low && !head_low) {
        forEachHighNeighbor(tail, {head}, nullptr, [&](auto neighborX) {
#if ENABLE_DEEPER_STATS
            ++op_uLv;
#endif
            operation(uLv, head->getId(), neighborX->getId(), 1);
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
void HStructureCounts::uLvVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                                       IncOrDecDoubleKey *operation2) {
    auto v_low_deg = vPart->getLowDegreeWithoutRemoved(v);
    auto max_low_deg = vPart->getTheoreticalMaxLowDegree();
    bool deleteAnchor_iterate_H{is_low(v) && vPart->get_num_high_deg() <
                                                 v_low_deg * max_low_deg};

    forEachHighNeighbor(v, {}, nullptr, [&](auto neighbor) {
        forEachHighNeighbor(v, {}, neighbor, [&](auto neighborY) {
            if (is_low(neighborY))
                return;

#if ENABLE_DEEPER_STATS
            ++op_uLv;
#endif
            operation1(uLv, neighbor->getId(), neighborY->getId(), 1);
        });
    });

    if (deleteAnchor_iterate_H) {
#if ENABLE_DEEPER_STATS
        op_uLv += vPart->get_num_high_deg();
#endif
        setZero(uLv, v);
    } else {
        forEachLowNeighbor(v, {}, nullptr, [&](auto neighbor) {
            forEachHighNeighbor(neighbor, {v}, nullptr, [&](auto neighborY) {
#if ENABLE_DEEPER_STATS
                ++op_uLv;
#endif
                operation2(uLv, v->getId(), neighborY->getId(), 1);
            });
        });
    }
}
