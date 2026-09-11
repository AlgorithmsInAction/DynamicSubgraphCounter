#include "graph/vertex.h"
#include "h_counts/HStructureCounts.h"
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
void HStructureCounts::tArcChange(const Algora::Arc *a,
                                  IncOrDecSingleKey *operation) {

    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    bool head_is_high{is_high(head)};
    bool tail_is_high{is_high(tail)};
    int increment{0};

    if (head_is_high && tail_is_high) {
        // Both head and tail are high, need to find all triangles
        // 1. Check for all high degree vertices if they connect to head and
        // tail, hence form a triangle
        // 2. Use uLv count for to check for all low degree vertices if a
        // triangle is formed

        // ldv - lower degree vertex
        // hdv - higher degree vertex
        vPart->forLowerHighDegree(head, tail, [&](auto ldv, auto hdv) {
            forEachHighNeighbor(ldv, {hdv}, nullptr, [&](auto h) {
                if (edge_exist(h, hdv)) {
                    operation(t, h->getId(), 1);
                    operation(t, ldv->getId(), 1);
                    operation(t, hdv->getId(), 1);
#if ENABLE_DEEPER_STATS
                    op_t += 3;
#endif
                }
            });
        });

        increment = this->getNruLv(head, tail);
        operation(t, head->getId(), increment);
        operation(t, tail->getId(), increment);

#if ENABLE_DEEPER_STATS
        op_t += 2;
#endif

    } else {
        // Iterate over low degree vertex with the lower degree to find t
        vPart->forLowLowerDegree(head, tail, [&](auto low, auto any) {
            bool any_is_high{is_high(any)};

            forEachHighNeighbor(low, {any}, nullptr, [&](auto neighbor) {
                if (!edge_exist(any, neighbor))
                    return;

#if ENABLE_DEEPER_STATS
                ++op_t;
#endif
                operation(t, neighbor->getId(), 1);
                if (any_is_high) {
#if ENABLE_DEEPER_STATS
                    ++op_t;
#endif
                    operation(t, any->getId(), 1);
                }
            });

            if (any_is_high) {
                forEachLowNeighbor(low, {any}, nullptr, [&](auto neighbor) {
                    if (!edge_exist(any, neighbor))
                        return;
#if ENABLE_DEEPER_STATS
                    ++op_t;
#endif
                    operation(t, any->getId(), 1);
                });
            }
        });
    }
}
/**
 * @brief updates the auxiliary count by calculating the number the count
 * should change by when the given vertex changes from one epsilon partition
 * to the other.
 *
 * @param v vertex that changes partition
 */
void HStructureCounts::tVertexChange(ILV *v) {

    if (is_low(v)) {
        setZero(t, v);
        return;
    }

    auto endPointOfRemoved = getLastRemovedIfAnyOtherThen(v);

    forEachNeighbor(v, {endPointOfRemoved}, [&](auto neighbor1) {
        forEachNeighbor(v, {endPointOfRemoved}, neighbor1, [&](auto neighbor2) {
            if (edge_exist(neighbor1, neighbor2)) {
#if ENABLE_DEEPER_STATS
                ++op_t;
#endif
                increaseOrCreate(t, v->getId(), 1);
            }
        });
    });
}
