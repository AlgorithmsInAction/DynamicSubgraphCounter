#pragma once

#include "ObserverAlgorithm.h"
#include "definitions.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph/arc.h"
#include "graph/vertex.h"
#include "graph/vertexpair.h"
#include "partition/EpsilonTab.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <functional>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

#define ENABLE_DEEPER_STATS 1

class StructureCounts : public ObserverAlgorithm {
  protected:
    StructureCounts() = default;
    virtual ~StructureCounts() = default;

    VertexPartition *vPart;
    std::vector<int> OnVertexUpId;
    std::vector<int> OnVertexDownId;
    std::vector<int> OnEpsArcAddId;
    std::vector<int> OnEpsArcRemoveId;

#if ENABLE_DEEPER_STATS
    std::chrono::duration<double> addTime{0};
    std::chrono::duration<double> delTime{0};
    std::chrono::duration<double> toHighTime{0};
    std::chrono::duration<double> toLowTime{0};
#endif

    friend class SubgraphCounts;
    friend class HSubgraphCounts;
    friend class ESubgraphCounts;

    virtual bool
    recompute(bool arcAdd,
              boost::unordered_map<const Algora::Vertex *, int> &arcBorder) = 0;
    virtual bool initVertexPartition() = 0;
    virtual bool prepare() = 0;
    virtual void cleanMaps() = 0;
    virtual void cleanup() = 0;

    VertexPartition *getVertexPartition() { return vPart; }

    VertexPartition *setVertexPartition(VertexPartition *partition) {
        return vPart = partition;
    }

    void printEpsTable() { vPart->print_table(); }

    ILV *
    getValidNeighbor(const Algora::Arc *arc, const ILV *currentVertex,
                     const std::vector<const ILV *> &excludeVertices = {}) {
        if (arc == vPart->getRemovedArc()) {
            return nullptr;
        }

        auto neighbor = CAST_ILV(arc->getOther(currentVertex));

        for (const auto &excludeVertex : excludeVertices) {
            if (neighbor == excludeVertex) {
                return nullptr;
            }
        }

        return neighbor;
    }

    template <typename Func>
    void forLowerDegree(ILV *v1, ILV *v2, Func operation) {

        int d1 = graph->getUndirectedDegree(v1);
        int d2 = graph->getUndirectedDegree(v2);

        if (d1 < d2) {
            operation(v1, v2);
        } else {
            operation(v2, v1);
        }
    }

    template <typename Func>
    void forLowerHighDegree(ILV *v1, ILV *v2, Func operation) {

        int d1 = vPart->getHighDegree(v1);
        int d2 = vPart->getHighDegree(v2);

        if (d1 < d2) {
            operation(v1, v2);
        } else {
            operation(v2, v1);
        }
    }

    template <typename Func>
    void forLowerLowDegree(ILV *v1, ILV *v2, Func operation) {

        int d1 = vPart->getLowDegree(v1);
        int d2 = vPart->getLowDegree(v2);

        if (d1 < d2) {
            operation(v1, v2);
        } else {
            operation(v2, v1);
        }
    }

    template <typename Func>
    void
    forEachTwoHighNeighbors(ILV *v,
                            const std::vector<const ILV *> &excludeVertices,
                            Func operation) {
        for (auto it1 = vPart->getHighDegBegin(v);
             it1 < vPart->getHighDegEnd(v); it1++) {
            auto neighborX = getValidNeighbor(*it1, v, excludeVertices);
            if (!neighborX) {
                continue;
            }
            for (auto it2 = vPart->getHighDegBegin(v);
                 it2 < vPart->getHighDegEnd(v); it2++) {
                auto neighborY = getValidNeighbor(*it2, v, excludeVertices);
                if (!neighborY) {
                    continue;
                }
                if (neighborX == neighborY) {
                    break;
                }
                operation(neighborX, neighborY);
            }
        }
    }

    template <typename Func>
    void forEachHighNeighbor(ILV *v,
                             const std::vector<const ILV *> &excludeVertices,
                             ILV *vStop, Func operation) {
        for (auto it1 = vPart->getHighDegBegin(v);
             it1 != vPart->getHighDegEnd(v); it1++) {
            auto neighbor = getValidNeighbor(*it1, v, excludeVertices);
            if (!neighbor) {
                continue;
            }
            if (neighbor == vStop) {
                break;
            }
            operation(neighbor);
        }
    }

    template <typename Func>
    void forEachLowNeighbor(ILV *v,
                            const std::vector<const ILV *> &excludeVertices,
                            ILV *vStop, Func operation) {
        for (auto it1 = vPart->getLowDegBegin(v); it1 != vPart->getLowDegEnd(v);
             it1++) {
            auto neighbor = getValidNeighbor(*it1, v, excludeVertices);
            if (!neighbor) {
                continue;
            }
            if (neighbor == vStop) {
                break;
            }
            operation(neighbor);
        }
    }

    template <typename Func>
    void forEachNeighbor(ILV *v,
                         const std::vector<const ILV *> &excludeVertices,
                         Func operation) {
        for (auto const &edge : v->getEdges()) {
            auto neighbor = getValidNeighbor(edge, v, excludeVertices);
            if (!neighbor) {
                continue;
            }
            operation(neighbor);
        }
    }

    template <typename Func>
    void forEachNeighbor(ILV *v,
                         const std::vector<const ILV *> &excludeVertices,
                         ILV *vStop, Func operation) {
        for (auto const &edge : v->getEdges()) {
            auto neighbor = getValidNeighbor(edge, v, excludeVertices);
            if (!neighbor) {
                continue;
            }
            if (neighbor == vStop) {
                break;
            }
            operation(neighbor);
        }
    }

    ILV *getLastRemovedIfAnyOtherThen(ILV *v) {

        ILV *endPointOfRemoved = nullptr;

        if (vPart->getRemovedArc() &&
            (vPart->getRemovedArc()->getFirst() == v ||
             vPart->getRemovedArc()->getSecond() == v))
            endPointOfRemoved = CAST_ILV(vPart->getRemovedArc()->getOther(v));

        return endPointOfRemoved;
    }
    template <typename Func>
    void forEachHighV(const std::vector<const ILV *> &excludeVertices,
                      const ILV *stopV, Func operation) {
        for (auto h = vPart->highDegVBegin(); h != vPart->highDegVEnd(); h++) {
            auto high = CAST_ILV(h->second);
            if (high == stopV) {
                break;
            }
            if (std::find(excludeVertices.begin(), excludeVertices.end(),
                          high) != excludeVertices.end())
                continue;
            operation(high);
        }
    }

    void setZero(SingleKeyMap &map, const Algora::Vertex *v) {
        map.erase(v->getId());
    }
    void setZero(DoubleKeyMap &map, const Algora::Vertex *v) {
        int first = v->getId();

        for (auto h = vPart->highDegVBegin(); h != vPart->highDegVEnd(); h++) {
            int second = h->first;
            if (first <= second)
                map.erase({first, second});

            else
                map.erase({second, first});
        }
    }

    inline bool edge_exist(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        return vPart->arcExists(v1, v2);
    }
    inline bool is_low(const Algora::Vertex *v) { return vPart->is_low_deg(v); }
    inline bool is_high(const Algora::Vertex *v) {
        return vPart->is_high_deg(v);
    }
};
