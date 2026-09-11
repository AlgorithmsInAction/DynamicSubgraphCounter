#include "partition/EpsilonTab.h"
#include "EpsilonTab.h"
#include "algorithm.basic.traversal/depthfirstsearch.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "graph/digraph.h"
#include "graph/graph_functional.h"
#include "graph/graphartifact.h"
#include "graph/vertex.h"
#include "observable.h"
#include "property/fastpropertymap.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <ostream>
#include <stdexcept>

/**
 * @brief Sets the value epsilon for the whole epsilon table
 *
 * @param epsilon
 * @return true
 * @return false
 */
bool EpsilonTab::setEpsilon(const double epsilon) {
    if ((epsilon <= 0) || (epsilon >= 1)) {
        std::cerr << "Epsilon must be between 0 and 1\n";
        return 0;
    }

    this->epsilon = epsilon;
    return 1;
}
void EpsilonTab::EpsTabOnVertexRemove(ILV *v) {
    HighDeg.erase(v->getId());
    if (round_robin_percent) {
        round_robin_size = std::ceil(round_robin_percent * graph->getSize());
    }
}

/**
 * @brief Updates the epsilon table when an arc a is inserted.
 * Maintains the ordering of neighbors according to high and low degree.
 * Checks if a recomputations is required.
 * Notifies observers of the new edge and any changes in the epsilon partition.
 *
 * @param a Edge that is newly inserted
 */
void EpsilonTab::EpsTabOnArcAdd(Algora::Arc *a) {
    DEBUG_PRINT("EpsTabOnArcAdd");

    removedArc = nullptr;
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    this->nr_edges += 1;

    if (notify_before_change) {
        adjacencyInsert(a);
        updateArcBorderOnInsert(a);
    }
    if (check_recompute(a, 1)) {
        return;
    }

    auto new_deg_head = graph->getUndirectedDegree(head);
    auto new_deg_tail = graph->getUndirectedDegree(tail);

    if (notify_before_change) {
        observableArcAdd.notifyObservers(a);

        if (new_deg_head >= 1.5 * theta) {
            insertVertexIntoHigh(head);
        }

        if (new_deg_tail >= 1.5 * theta) {
            insertVertexIntoHigh(tail);
        }

        if (lazy_recompute || delayed_recompute) {
            if (new_deg_head < 0.5 * theta) {
                removeVertexFromHigh(head);
            }

            if (new_deg_tail < 0.5 * theta) {
                removeVertexFromHigh(tail);
            }
        }
    } else {
        auto head_into_H = new_deg_head >= 1.5 * theta;
        auto tail_into_H = new_deg_tail >= 1.5 * theta;

        auto head_into_L = 0;
        auto tail_into_L = 0;

        if (lazy_recompute || delayed_recompute) {
            head_into_L = new_deg_head < 0.5 * theta;
            tail_into_L = new_deg_tail < 0.5 * theta;
        }
        // if we need a part change
        if (head_into_H || tail_into_H ||
            ((lazy_recompute || delayed_recompute) &&
             (head_into_L || tail_into_L))) {
            fixTable();
            graph->deactivateEdge(a);
            unfixTable();

            if (head_into_H) {
                insertVertexIntoHigh(head);
            }
            if (tail_into_H) {
                insertVertexIntoHigh(tail);
            }

            if (lazy_recompute || delayed_recompute) {
                if (head_into_L) {
                    removeVertexFromHigh(head);
                }
                if (tail_into_L) {
                    removeVertexFromHigh(tail);
                }
            }

            fixTable();
            graph->activateEdge(a);
            unfixTable();
        }
        adjacencyInsert(a);
        updateArcBorderOnInsert(a);

        observableArcAdd.notifyObservers(a);
    }

    unfixTable();
}

/**
 * @brief Updates the epsilon table when an arc a is removed.
 * Maintains the ordering of neighbors according to high and low degree.
 * Checks if a recomputations is required.
 * Notifies observers of the new edge and any changes in the epsilon partition.
 *
 * @param a Edge that is newly removed
 */
void EpsilonTab::EpsTabOnArcRemove(Algora::Arc *a) {
    ILV *head = CAST_ILV(a->getHead());
    ILV *tail = CAST_ILV(a->getTail());
    this->nr_edges -= 1;

    updateArcBorderOnRemove(a);

    if (check_recompute(a, 0)) {
        return;
    }

    observableArcRemove.notifyObservers(a);

    auto new_deg_head = graph->getUndirectedDegree(head) - 1;
    auto new_deg_tail = graph->getUndirectedDegree(tail) - 1;

    if (new_deg_head < 0.5 * theta) {
        removeVertexFromHigh(head, tail);
    }
    if (new_deg_tail < 0.5 * theta) {
        removeVertexFromHigh(tail, head);
    }

    if (lazy_recompute || delayed_recompute) {
        if (new_deg_head >= 1.5 * theta) {
            insertVertexIntoHigh(head);
        }

        if (new_deg_tail >= 1.5 * theta) {
            insertVertexIntoHigh(tail);
        }
    }
}

/**
 * @brief Checks if all required setting were set and creates initializes the
 * Epsilon table.
 *
 * @return true
 * @return false
 */
bool EpsilonTab::prepare() {
    if (epsilon == -1) {
        std::cerr << "Must set epsilon before setting the Graph\n";
        return 0;
    }

    nr_edges = graph->getNumArcs(0);
    M = 2 * nr_edges;
    // theta = std::pow(epsilon, M);
    theta = std::pow(M, epsilon);

    graph->onVertexRemove(&OnVertexRemoveId.emplace_back(0),
                          [&](Algora::Vertex *v) {
                              if (fixed)
                                  return;

                              EpsTabOnVertexRemove(CAST_ILV(v));
                          });

    graph->onVertexAdd(&OnVertexAddId.emplace_back(0), [&](Algora::Vertex *v) {
        if (fixed)
            return;

        if (round_robin_percent) {
            round_robin_size =
                std::ceil(round_robin_percent * graph->getSize());
        }
    });

    // If an edge gets inserted, and the table is not fixed, start time
    // measurements, maintain the adjacency-map that keeps track of all current
    // edges and call EpsTabOnArcAdd(a), which maintains the epsilon table and
    // notifies other observers.
    graph->onArcAdd(&OnArcAddId.emplace_back(0), [&](Algora::Arc *a) {
        if (fixed)
            return;
        ++edge_insertions;
        updateStatsStart();
        auto startTimeDyn = std::chrono::high_resolution_clock::now();

        EpsTabOnArcAdd(a);
        if (round_robin_size || !wronglyPartitioned.empty()) {
            round_robin();
        }

        auto endTimeDyn = std::chrono::high_resolution_clock::now();
        currentDynTime = endTimeDyn - startTimeDyn;
        updateStats();
    });

    // If an edge gets removed, and the table is not fixed, start time
    // measurements, maintain the adjacency-map that keeps track of all current
    // edges and call EpsTabOnArcAdd(a), which maintains the epsilon table and
    // notifies other observers.
    graph->onArcRemove(&OnArcRemoveId.emplace_back(0), [&](Algora::Arc *a) {
        if (fixed)
            return;

        ++edge_removals;
        updateStatsStart();
        auto startTimeDyn = std::chrono::high_resolution_clock::now();
        removedArc = a;

        adjacencyDelete(a);
        EpsTabOnArcRemove(a);
        if (round_robin_size || !wronglyPartitioned.empty()) {
            round_robin();
        }

        removedArc = nullptr;
        auto endTimeDyn = std::chrono::high_resolution_clock::now();
        currentDynTime = endTimeDyn - startTimeDyn;
        updateStats();
    });

    return 1;
}

void EpsilonTab::cleanup() {
    HighDeg.clear();
    arcBorder.clear();
}

void EpsilonTab::round_robin() {
    if (delayed_recompute) {
        if (wronglyPartitioned.front()) {
            return;
        }
        auto i = 0;
        while (i < adjust_per_update) {
            auto v = wronglyPartitioned.front();
            auto deg = graph->getUndirectedDegree(v) -
                       (getRemovedArc() && (getRemovedArc()->getFirst() == v ||
                                            getRemovedArc()->getSecond() == v));
            auto vertex_belongs_to_H = deg >= theta;
            auto vertex_is_in_H = is_high_deg(v);

            if (vertex_belongs_to_H && !vertex_is_in_H) {
                insertVertexIntoHigh(v);
            } else if (!vertex_belongs_to_H && vertex_is_in_H) {
                removeVertexFromHigh(v);
            }
            wronglyPartitioned.pop_front();
            ++i;
        }
    } else if (lazy_recompute) {
        for (u_int32_t i = 0; i < round_robin_size; ++i) {
            round_robin_id = (round_robin_id + 1) % graph->getSize();

            auto v = CAST_ILV(graph->vertexAt(round_robin_id));

            auto deg = graph->getUndirectedDegree(v) -
                       (getRemovedArc() && (getRemovedArc()->getFirst() == v ||
                                            getRemovedArc()->getSecond() == v));
            auto vertex_belongs_to_H = deg >= theta;
            auto vertex_is_in_H = is_high_deg(v);

            if (vertex_belongs_to_H && !vertex_is_in_H) {
                insertVertexIntoHigh(v);
            } else if (!vertex_belongs_to_H && vertex_is_in_H) {
                removeVertexFromHigh(v);
            }
        }
    }
}

/**
 * @brief Checks if the conditions for a recomputation are met and starts it if
 * needed. Also starts time measurements for the recomputation
 *
 * @param a Edge used in the last operation
 * @param arcAdd if the last edge was inserted or deleted
 * @return true
 * @return false
 */
bool EpsilonTab::check_recompute(Algora::Arc *a, bool arcAdd) {
    if ((nr_edges < std::floor(M / (recomputeFactor * recomputeFactor))) ||
        (nr_edges >= M)) {
        auto startTimeRecompute = std::chrono::high_resolution_clock::now();

        auto recomputed = recompute(a, arcAdd);

        auto endTimeRecompute = std::chrono::high_resolution_clock::now();
        totalRecomputationTime += endTimeRecompute - startTimeRecompute;
        return recomputed;
    }
    return 0;
}
/**
 * @brief Starts the recomputation by recalculating the epsilon partition and
 * calling the provided "on_recompute_func"
 *
 * @param a Edge used in the last operation
 * @param arcAdd if the last edge was inserted or deleted
 * @return true
 * @return false
 */
bool EpsilonTab::recompute(Algora::Arc *a, bool arcAdd) {
    ++num_recomputations;
    DEBUG_PRINT("Recomputation %d", num_recomputations);

    // Compute a new theta
    M = recomputeFactor * nr_edges;
    theta = std::max(std::pow(M, epsilon), 1.0);

    // lazy recompute -> only change when a vertex is touched
    // soft recompute -> only change incorrect partitioned vertices
    // normal recompute -> recompute from scratch
    if (lazy_recompute) {
        return false;
    }

    if (delayed_recompute) {
        graph->mapVertices([&](Algora::Vertex *vertex) {
            auto v = CAST_ILV(vertex);
            auto corrected =
                (!notify_before_change) && arcAdd &&
                (a->getFirst() == vertex || a->getSecond() == vertex);
            auto deg = graph->getUndirectedDegree(v) + corrected;
            auto vertex_belongs_to_H = deg >= theta;
            auto vertex_is_in_H = is_high_deg(v);

            if (vertex_belongs_to_H && !vertex_is_in_H ||
                !vertex_belongs_to_H && vertex_is_in_H) {
                wronglyPartitioned.push_back(v);
            }
        });
        adjust_per_update = std::ceil(wronglyPartitioned.size() /
                                      (nr_edges * (1 - 1 / recomputeFactor)));

        return false;
    }

    if (a != nullptr) {
        if (arcAdd) {
            if (notify_before_change) {
                observableArcAdd.notifyObservers(a);
            } else {
                fixTable();
                graph->deactivateEdge(a);
                unfixTable();
            }
        } else
            observableArcRemove.notifyObservers(a);
    }

    if (soft_recompute) {
        graph->mapVertices([&](Algora::Vertex *vertex) {
            auto v = CAST_ILV(vertex);
            auto corrected =
                (!notify_before_change) && arcAdd &&
                (a->getFirst() == vertex || a->getSecond() == vertex);
            auto deg = graph->getUndirectedDegree(v) + corrected;
            auto vertex_belongs_to_H = deg >= theta;
            auto vertex_is_in_H = is_high_deg(v);

            if (vertex_belongs_to_H && !vertex_is_in_H) {
                insertVertexIntoHigh(v);
            } else if (!vertex_belongs_to_H && vertex_is_in_H) {
                removeVertexFromHigh(v);
            }
        });
    } else {
        this->cleanup();

        graph->mapVertices([&](Algora::Vertex *v) {
            auto corrected = (!notify_before_change) && arcAdd &&
                             (a->getFirst() == v || a->getSecond() == v);
            if (graph->getUndirectedDegree(v) + corrected >= theta) {
                HighDeg.insert({v->getId(), v});
            }
        });
        if (on_recompute_func != nullptr)
            on_recompute_func(arcAdd, arcBorder);
    }

    if (arcAdd) {
        if (!notify_before_change) {
            fixTable();
            graph->activateEdge(a);
            unfixTable();

            adjacencyInsert(a);
            updateArcBorderOnInsert(a);

            observableArcAdd.notifyObservers(a);
        }
    }

    return true;
}

/**
 * @brief Print all vertices with a high degree
 *
 */
void EpsilonTab::print_table() {
    if (!hasGraph()) {
        std::cerr << "No Graph set\n";
        return;
    }
    std::cout << "High Degree Table: \n";

    for (auto v : HighDeg)
        std::cout << v.first << " ";
}
