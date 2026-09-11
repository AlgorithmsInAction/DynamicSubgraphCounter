#include "HIndex.h"
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

/**
 * @brief Maintains the h-indes when a vertex gets added. Also updates the
 * partition into high and low degree.
 *
 * @param v vertex that is added
 * @param deg degree of vertex v
 */
void HIndex::VertexAdd(Algora::Vertex *v, int deg) {

    if ((int)VerticesOfDeg.size() <= deg)
        VerticesOfDeg.resize(deg + 1);
    VerticesOfDeg.at(deg).insert({v->getId(), v});
    auto h_index_before = HelperH.size();

    if (deg <= (int)h_index_before) {
        return;
    }

    // Only insert into HighDeg if deg is at least gradual_factor*h_index
    if (deg >= gradual_factor * h_index_before) {
        insertVertexIntoHigh(CAST_ILV(v));
    }
    HelperH.insert({v->getId(), v});

    if (!Bubble.empty()) {
        // h-index does not change, swap v with a vertex in the bubble
        int to_remove_id = Bubble.begin()->first;
        auto to_remove_ptr = Bubble.begin()->second;
        Bubble.erase(Bubble.begin());
        HelperH.erase(to_remove_ptr->getId());

        // If to_remove_ptr is part of HighDeg, remove it
        if (is_high_deg(to_remove_ptr->getId())) {
            removeVertexFromHigh(CAST_ILV(to_remove_ptr), nullptr);
        }

        VerticesOfDeg.at(graph->getUndirectedDegree(to_remove_ptr))
            .insert({to_remove_id, to_remove_ptr});
        if (deg == (int)h_index_before)
            Bubble.insert({v->getId(), v});

    } else {
        // h-index increases by one
        auto h_index_new = HelperH.size();
        Bubble.clear();

        if (!VerticesOfDeg.at(h_index_new).empty()) {
            Bubble.insert(VerticesOfDeg.at(h_index_new).begin(),
                          VerticesOfDeg.at(h_index_new).end());
            VerticesOfDeg.at(h_index_new).clear();
        }
    }
}
/**
 * @brief Maintains the h-indes when a vertex gets removed. Also updates the
 * partition into high and low degree.
 *
 * @param v vertex that is removed
 * @param deg degree of vertex v
 */
void HIndex::VertexRemove(Algora::Vertex *v, int deg) {
    auto h_index_before = HelperH.size();

    if (VerticesOfDeg.size() <= h_index_before)
        VerticesOfDeg.resize(h_index_before + 1);
    Bubble.erase(v->getId());
    VerticesOfDeg.at(deg).erase(v->getId());

    if (!HelperH.contains(v->getId())) {
        // no changes to the h-index
        return;
    }

    HelperH.erase(v->getId());

    // If v is part of HighDeg, remove it
    if (is_high_deg(v->getId())) {
        removeVertexFromHigh(CAST_ILV(v),
                             removedArc != nullptr
                                 ? CAST_ILV(removedArc->getOther(v))
                                 : nullptr);
    }

    if (!VerticesOfDeg.at(h_index_before).empty()) {
        // h -index does not change, replace v with a vertex of deg=h-index

        int key = VerticesOfDeg.at(h_index_before).begin()->first;
        auto val = VerticesOfDeg.at(h_index_before).begin()->second;
        Bubble.insert({key, val});
        HelperH.insert({val->getId(), val});

        // Only insert into HighDeg if deg is at least gradual_factor*h_index
        if (graph->getUndirectedDegree(val) >=
            gradual_factor * h_index_before) {
            insertVertexIntoHigh(CAST_ILV(val));
        }
        VerticesOfDeg.at(h_index_before).erase(key);
    } else {
        // h -index reduced by 1, store bubble in VerticesOfDeg and clear it

        VerticesOfDeg.at(h_index_before).insert(Bubble.begin(), Bubble.end());
        Bubble.clear();
    }

    if ((VerticesOfDeg.end() - 1)->empty())
        VerticesOfDeg.erase(VerticesOfDeg.end() - 1);
}
/**
 * @brief Maintains the h-indes when a vertex gets added. Also updates the
 * partition into high and low degree.
 *
 * @param v vertex that is added
 */
void HIndex::EpsTabOnVertexAdd(Algora::Vertex *v) {
    if (fixed)
        return;
    VertexAdd(v, graph->getUndirectedDegree(v));
}
/**
 * @brief Maintains the h-indes when a vertex gets added. Also updates the
 * partition into high and low degree.
 *
 * @param v vertex that is added
 */
void HIndex::EpsTabOnVertexRemove(ILV *v) {
    if (fixed)
        return;

    VertexRemove(v, graph->getUndirectedDegree(v));
}

/**
 * @brief Maintains internal structures for a vertex that stays in H on a degree
 * update
 *
 * @param v vertex whose degree changes
 * @param deg_before degree the vertex v currently has
 * @param deg_new new degree the vertex v
 */
void HIndex::AdjustDegOfVertexInH(Algora::Vertex *v, int deg_before,
                                  int deg_new) {
    int h_index = HelperH.size();
    if (deg_before == h_index) {
        if (static_cast<int>(VerticesOfDeg.size()) <= deg_new)
            VerticesOfDeg.resize(deg_new + 1);
        Bubble.erase(v->getId());
        VerticesOfDeg.at(deg_new).insert({v->getId(), v});
    } else if (deg_new == h_index) {
        VerticesOfDeg.at(deg_before).erase(v->getId());
        Bubble.insert({v->getId(), v});
    } else {
        if (static_cast<int>(VerticesOfDeg.size()) <= deg_new)
            VerticesOfDeg.resize(deg_new + 1);

        VerticesOfDeg.at(deg_before).erase(v->getId());
        VerticesOfDeg.at(deg_new).insert({v->getId(), v});
    }

    // Only insert into HighDeg if deg is at least gradual_factor*h_index
    if (!is_high_deg(v) && (deg_new >= gradual_factor * h_index)) {
        insertVertexIntoHigh(CAST_ILV(v));
    }
}

/**
 * @brief Maintains the h-index and partitioning into high and low degree
 * vertices when a new edge gets inserted and then calls all observers. The time
 * measurements are also done here.
 *
 * @param a Edge that gets inserted
 */
void HIndex::EpsTabOnArcAdd(Algora::Arc *a) {
    if (fixed)
        return;

    ++edge_insertions;
    updateStatsStart();
    auto startTimeDyn = std::chrono::high_resolution_clock::now();
    auto head = a->getFirst();
    auto tail = a->getSecond();

    fixTable();
    graph->deactivateEdge(a);
    unfixTable();

    // Changing the value of the degree may be accomplished by deleting the
    // vertex and then reinserting it.
    // Except, when it was in H and stays in H (Note that it always stays in H
    // on an edge insert, so only check the first condition)
    auto remove_old = [&](auto v) {
        int oldDeg = graph->getUndirectedDegree(v);
        bool is_in_H = HelperH.contains(v->getId());
        if (!is_in_H && oldDeg)
            VertexRemove(v, oldDeg);
    };
    auto adjust_vertex = [&](auto v) {
        int oldDeg = graph->getUndirectedDegree(v);
        int newDeg = oldDeg + 1;
        bool is_in_H = HelperH.contains(v->getId());
        if (is_in_H) {
            AdjustDegOfVertexInH(v, oldDeg, newDeg);
        } else {
            VertexAdd(v, newDeg);
        }
    };
    remove_old(head);
    remove_old(tail);

    adjust_vertex(head);
    adjust_vertex(tail);

    fixTable();
    graph->activateEdge(a);
    unfixTable();
    adjacencyInsert(a);
    updateArcBorderOnInsert(a);

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
void HIndex::EpsTabOnArcRemove(Algora::Arc *a) {
    if (fixed)
        return;
    ++edge_removals;
    updateStatsStart();
    removedArc = a;

    auto startTimeDyn = std::chrono::high_resolution_clock::now();
    auto head = a->getFirst();
    auto tail = a->getSecond();

    updateArcBorderOnRemove(a);
    observableArcRemove.notifyObservers(a);

    // Changing the value of the degree may be accomplished by deleting the
    // vertex and then reinserting it.
    // Except, when it was in H and stays in H
    auto adjust_vertex = [&](auto v) {
        int oldDeg = graph->getUndirectedDegree(v);
        int newDeg = oldDeg - 1;
        int h_index = HelperH.size();
        bool is_in_H = HelperH.contains(v->getId());
        bool stays_in_H = newDeg >= h_index;
        if (is_in_H && stays_in_H) {
            AdjustDegOfVertexInH(v, oldDeg, newDeg);
        } else {
            VertexRemove(v, oldDeg);
        }
    };
    auto add_new = [&](auto v) {
        int oldDeg = graph->getUndirectedDegree(v);
        int newDeg = oldDeg - 1;
        int h_index = HelperH.size();
        bool is_in_H = HelperH.contains(v->getId());
        bool stays_in_H = newDeg >= h_index;

        if (!(is_in_H && stays_in_H) && newDeg)
            VertexAdd(v, newDeg);
    };

    adjust_vertex(head);
    adjust_vertex(tail);

    add_new(head);
    add_new(tail);

    removedArc = nullptr;

    auto endTimeDyn = std::chrono::high_resolution_clock::now();
    currentDynTime = endTimeDyn - startTimeDyn;
    updateStats();
}

/**
 * @brief Prepares the object. This includes linking with the observers
 * of the graph given. Maintains the adjacency map that contains all
 * presently active arcs
 *
 */
bool HIndex::prepare() {
    graph->onVertexRemove(
        &OnVertexRemoveId.emplace_back(0),
        [&](Algora::Vertex *v) { EpsTabOnVertexRemove(CAST_ILV(v)); });
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
void HIndex::cleanup() { HighDeg.clear(); }

void HIndex::print_table() {
    if (!hasGraph()) {
        std::cerr << "No Graph set\n";
        return;
    }

    std::cout << "High Degree H: \n";
    for (auto v : HighDeg)
        std::cout << v.first << " ";
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Helper H: \n";
    for (auto v : HelperH)
        std::cout << v.first << " ";
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "B: \n";
    for (auto v : Bubble)
        std::cout << v.first << " ";
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "C: \n";
    int count{0};
    for (auto c : VerticesOfDeg) {
        std::cout << "deg " << count << ": ";
        for (auto v : c)
            std::cout << v.first << " ";

        count++;
        std::cout << "\n";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "\n\n";
}