#include "MockPartition.h"
#include "algorithm.basic.traversal/depthfirstsearch.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph/arc.h"
#include "graph/digraph.h"
#include "graph/graph_functional.h"
#include "graph/graphartifact.h"
#include "graph/vertex.h"
#include "observable.h"
#include "property/fastpropertymap.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>

void MockPartition::EpsTabOnVertexRemove(ILV *v) {}

/**
 * @brief Maintains the h-index and partitioning into high and low degree
 * vertices when a new edge gets inserted and then calls all observers. The time
 * measurements are also done here.
 *
 * @param a Edge that gets inserted
 */
void MockPartition::EpsTabOnArcAdd(Algora::Arc *a) {
    if (fixed)
        return;

    ++edge_insertions;
    updateStatsStart();
    auto startTimeDyn = std::chrono::high_resolution_clock::now();

    adjacencyInsert(a);
    observableArcAdd.notifyObservers(a);

    auto endTimeDyn = std::chrono::high_resolution_clock::now();
    currentDynTime = endTimeDyn - startTimeDyn;
    updateStats();
}
/**
 * @brief Maintains the h-index and partitioning into high and low
 * degree vertices when an edge gets removed and then calls all
 * observers. The time measurements are also done here.
 *
 * @param a Edge that gets removed
 */
void MockPartition::EpsTabOnArcRemove(Algora::Arc *a) {
    if (fixed)
        return;
    ++edge_removals;
    updateStatsStart();
    removedArc = a;

    auto startTimeDyn = std::chrono::high_resolution_clock::now();

    observableArcRemove.notifyObservers(a);

    auto endTimeDyn = std::chrono::high_resolution_clock::now();
    currentDynTime = endTimeDyn - startTimeDyn;
    updateStats();
    removedArc = nullptr;
}

/**
 * @brief Prepares the object. This includes linking with the observers
 * of the graph given. Maintains the adjacency map that contains all
 * presently active arcs
 *
 */
bool MockPartition::prepare() {
    graph->onArcAdd(&OnArcAddId.emplace_back(0),
                    [&](Algora::Arc *a) { EpsTabOnArcAdd(a); });

    graph->onArcRemove(&OnArcRemoveId.emplace_back(0), [&](Algora::Arc *a) {
        adjacencyDelete(a);
        EpsTabOnArcRemove(a);
    });

    return 1;
}

/**
 * @brief Resets the partitioning. All vertices will be low degree.
 *
 */
void MockPartition::cleanup() { HighDeg.clear(); }

void MockPartition::print_table() {}