#include "graph/vertex.h"
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
 * @param recompute_after_removed true if this function is called as part of a
 * recomputation after an edge has been removed
 */
void HVStructureCounts::cLVArcChange(const Algora::Arc *a,
                                     IncOrDecDoubleKey *operation,
                                     bool recompute_after_removed) {

    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    const bool headHigh{is_high(head)};
    const bool tailHigh{is_high(tail)};

    int deg_t{0}, deg_h{0};

    if (recompute_after_removed) {
        deg_h = vPart->getDegreeWithoutRemoved(head);
        deg_t = vPart->getDegreeWithoutRemoved(tail);
    } else {
        deg_h = graph->getUndirectedDegree(head);
        deg_t = graph->getUndirectedDegree(tail);
    }

    if (!headHigh) {
        forEachNeighbor(head, {tail}, nullptr, [&](auto neighborX) {
#if ENABLE_DEEPER_STATS
            ++op_cLV;
#endif
            operation(cLV, tail->getId(), neighborX->getId(), deg_h - 2);

            forEachNeighbor(head, {tail}, neighborX, [&](auto neighborY) {
#if ENABLE_DEEPER_STATS
                ++op_cLV;
#endif
                operation(cLV, neighborX->getId(), neighborY->getId(), 1);
            });
        });
    }
    if (!tailHigh) {

        forEachNeighbor(tail, {head}, nullptr, [&](auto neighborX) {
#if ENABLE_DEEPER_STATS
            ++op_cLV;
#endif
            operation(cLV, head->getId(), neighborX->getId(), deg_t - 2);

            forEachNeighbor(tail, {head}, neighborX, [&](auto neighborY) {
#if ENABLE_DEEPER_STATS
                ++op_cLV;
#endif
                operation(cLV, neighborX->getId(), neighborY->getId(), 1);
            });
        });
    }
}

void HVStructureCounts::cLVVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                                        IncOrDecDoubleKey *operation2) {
    int deg_v = vPart->getDegreeWithoutRemoved(v);

    // Find cLV with v as non-anchor
    forEachNeighbor(v, {}, nullptr, [&](auto x) {
        forEachNeighbor(v, {}, x, [&](auto y) {
#if ENABLE_DEEPER_STATS
            ++op_cLV;
#endif
            operation1(cLV, x->getId(), y->getId(), deg_v - 2);
        });
    });
}
