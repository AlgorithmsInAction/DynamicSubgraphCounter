#include "h_counts/HStructureCounts.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "graph/graph_functional.h"
#include "graph/vertex.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <iostream>
#include <utility>
#include <vector>

/**
 * @brief Create the epsilon table with the given value for epsilon
 *
 * @return true
 * @return false
 */
bool HStructureCounts::initVertexPartition() {
    if (!hasGraph()) {
        std::cerr << "Need to set Graph before creation\n";
        return 0;
    }
    vPart->cleanup();
    vPart->setGraph(graph);
    if (!vPart->prepare())
        return 0;
    vPart->setOnRecompute(
        [&](bool arcAdd,
            boost::unordered_map<const Algora::Vertex *, int> &arcBorder) {
            this->cleanMaps();
            this->recompute(arcAdd, arcBorder);
        });
    return 1;
}

/**
 * @brief Recompute all Auxiliary Counts after the Epsilon Table has been
 * recalculated. This is done by first fixing the Epsilon Table, deactivating
 * all inserted edges and than inserting them again and calculating the
 * Auxiliary Counts.
 *
 * @param arcAdd If the last operation inserted an edge
 * @param arcBorder Hash map containing for each vertex the border in the
 * incidence list, where the edges only lead to low degree vertices
 * @return true
 * @return false
 */
bool HStructureCounts::recompute(
    bool arcAdd, boost::unordered_map<const Algora::Vertex *, int> &arcBorder) {

    vPart->fixTable();
    std::vector<Algora::Arc *> arcs;
    arcs.reserve(vPart->get_m());

    graph->mapEdges([&](auto e) {
        if (e == vPart->getRemovedArc())
            return;
        arcs.push_back(e);
    });

    for (auto it = arcs.begin(); it != arcs.end(); it++) {
        graph->deactivateEdge(*it);
    }

    vPart->adjacencyDeleteAll();

    for (auto it = arcs.begin(); it != arcs.end(); it++) {

        graph->activateEdge(*it);

        auto head = CAST_ILV((*it)->getHead());
        auto tail = CAST_ILV((*it)->getTail());

        vPart->adjacencyInsert(*it);

        if (is_high(head)) {
            tail->swapEdges(tail->undirectedIndexOf(*it), arcBorder[tail]++);
        }
        if (is_high(tail)) {
            head->swapEdges(head->undirectedIndexOf(*it), arcBorder[head]++);
        }

        if (objectives["vLV"])
            vLVArcChange(*it, increaseOrCreate, !arcAdd);
        if (objectives["uLv"])
            uLvArcChange(*it, increaseOrCreate);
        if (objectives["t"])
            tArcChange(*it, increaseOrCreate);
        if (objectives["uLLv"])
            uLLvArcChange(*it, increaseOrCreate);
        if (objectives["cLV"])
            cLVArcChange(*it, increaseOrCreate, !arcAdd);
        if (objectives["pLL"])
            pLLArcChange(*it, increaseOrCreate);
        if (objectives["uHv"])
            uHvArcChange(*it, increaseOrCreate);
        if (objectives["cL"])
            cLArcChange(*it, increaseOrCreate);
    }
    vPart->unfixTable();

    return 1;
}

/**
 * @brief Sets which Auxiliary Counts need to be calculated
 *
 * @param vLV
 * @param t
 * @param uLv
 * @param uLLv
 * @param cLV
 * @param pLL
 * @param uHv
 * @param cL
 */
void HStructureCounts::setObjectives(bool vLV, bool t, bool uLv, bool uLLv,
                                     bool cLV, bool pLL, bool uHv, bool cL) {
    objectives["vLV"] = vLV;
    objectives["t"] = t;
    objectives["uLv"] = uLv;
    objectives["uLLv"] = uLLv;
    objectives["cLV"] = cLV;
    objectives["pLL"] = pLL;
    objectives["uHv"] = uHv;
    objectives["cL"] = cL;
}

/**
 * @brief Initializes the Auxiliary Counts by adding the functions, that
 * maintain the Counts to the respective observers.
 *
 * @return true
 * @return false
 */
bool HStructureCounts::prepare() {
    if (!hasGraph() || !vPart->hasGraph()) {
        std::cerr << "Need to set Graph and VertexPartition before preparing\n";
        return 0;
    }
    if (objectives.empty()) {
        std::cerr << "Need to set Objectives before preparing\n";
        return 0;
    }

    // Fill vLV
    if (objectives["vLV"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(0), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            DEBUG_PRINT("onArcAdd (%d,%d)", a->getFirst()->getId(),
                        a->getSecond()->getId());
            vLVArcChange(a, increaseOrCreate, false);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(0), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                DEBUG_PRINT("onArcRemove (%d,%d)", a->getFirst()->getId(),
                            a->getSecond()->getId());
                vLVArcChange(a, reduceOrDelete, false);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(0), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                DEBUG_PRINT("onVertexToHigh (%d)", v->getId());
                vLVVertexChange(CAST_ILV(v), reduceOrDelete, increaseOrCreate);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(0), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                DEBUG_PRINT("onVertexToLow (%d)", v->getId());
                vLVVertexChange(CAST_ILV(v), increaseOrCreate, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill uLv
    if (objectives["uLv"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(1), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            uLvArcChange(a, increaseOrCreate);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(1), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uLvArcChange(a, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(1), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uLvVertexChange(CAST_ILV(v), reduceOrDelete, increaseOrCreate);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(1), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uLvVertexChange(CAST_ILV(v), increaseOrCreate, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill t (needs to be after uLv)
    if (objectives["t"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(2), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            tArcChange(a, increaseOrCreate);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(2), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                tArcChange(a, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(2), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                tVertexChange(CAST_ILV(v));
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(2), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                tVertexChange(CAST_ILV(v));
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill uLLv
    if (objectives["uLLv"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(3), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            uLLvArcChange(a, increaseOrCreate);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(3), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uLLvArcChange(a, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(3), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uLLvVertexChange(CAST_ILV(v), reduceOrDelete, increaseOrCreate);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(3), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uLLvVertexChange(CAST_ILV(v), increaseOrCreate, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill cLV
    if (objectives["cLV"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(4), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            cLVArcChange(a, increaseOrCreate, false);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(4), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                cLVArcChange(a, reduceOrDelete, false);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(4), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                cLVVertexChange(CAST_ILV(v), reduceOrDelete, increaseOrCreate);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(4), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                cLVVertexChange(CAST_ILV(v), increaseOrCreate, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill pLL
    if (objectives["pLL"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(5), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            pLLArcChange(a, increaseOrCreate);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(5), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                pLLArcChange(a, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(5), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                pLLVertexChange(CAST_ILV(v), reduceOrDelete, increaseOrCreate);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(5), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                pLLVertexChange(CAST_ILV(v), increaseOrCreate, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill uHv
    if (objectives["uHv"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(6), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            uHvArcChange(a, increaseOrCreate);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(6), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uHvArcChange(a, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(6), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uHvVertexChange(CAST_ILV(v), increaseOrCreate);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(6), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                uHvVertexChange(CAST_ILV(v), reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    // Fill cL
    if (objectives["cL"]) {
        vPart->onArcAdd(&OnEpsArcAddId.emplace_back(7), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
            auto start = std::chrono::high_resolution_clock::now();
#endif
            cLArcChange(a, increaseOrCreate);
#if ENABLE_DEEPER_STATS
            auto end = std::chrono::high_resolution_clock::now();
            addTime += end - start;
#endif
        });
        vPart->onArcRemove(
            &OnEpsArcRemoveId.emplace_back(7), [&](Algora::Arc *a) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                cLArcChange(a, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                delTime += end - start;
#endif
            });
        vPart->onVertexToHigh(
            &OnVertexUpId.emplace_back(7), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                cLVertexChange(CAST_ILV(v), reduceOrDelete, increaseOrCreate);

#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toHighTime += end - start;
#endif
            });
        vPart->onVertexToLow(
            &OnVertexDownId.emplace_back(7), [&](Algora::Vertex *v) {
#if ENABLE_DEEPER_STATS
                auto start = std::chrono::high_resolution_clock::now();
#endif
                cLVertexChange(CAST_ILV(v), increaseOrCreate, reduceOrDelete);
#if ENABLE_DEEPER_STATS
                auto end = std::chrono::high_resolution_clock::now();
                toLowTime += end - start;
#endif
            });
    }

    return 1;
}

void HStructureCounts::cleanMaps() {
    this->vLV.clear();
    this->uLv.clear();
    this->t.clear();
    this->uLLv.clear();
    this->cLV.clear();
    this->pLL.clear();
    this->uHv.clear();
    this->cL.clear();
}
/**
 * @brief Remove all Functions from the observers.
 *
 */
void HStructureCounts::cleanup() {
    cleanMaps();
    for (auto &id : OnVertexUpId)
        vPart->removeOnVertexToHigh(&id);
    for (auto &id : OnVertexDownId)
        vPart->removeOnVertexToLow(&id);
    for (auto &id : OnEpsArcAddId)
        vPart->removeOnArcAdd(&id);
    for (auto &id : OnEpsArcRemoveId)
        vPart->removeOnArcRemove(&id);
}
