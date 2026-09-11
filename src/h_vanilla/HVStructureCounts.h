#pragma once

#include "StructureCounts.h"
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

class HVStructureCounts : public StructureCounts {
  public:
    HVStructureCounts() = default;
    ~HVStructureCounts() override = default;

  protected:
    SingleKeyMap vLV;
    DoubleKeyMap uLv;
    SingleKeyMap t;
    DoubleKeyMap uLLv;
    DoubleKeyMap cLV;
    DoubleKeyMap pLL;
    DoubleKeyMap uHv;
    TripleKeyMap cL;
    boost::unordered_flat_map<std::string, bool> objectives;

#if ENABLE_DEEPER_STATS
    size_t op_vLV{0};
    size_t op_uLv{0};
    size_t op_t{0};
    size_t op_uLLv{0};
    size_t op_cLV{0};
    size_t op_pLL{0};
    size_t op_uHv{0};
    size_t op_cL{0};
#endif

    friend class SubgraphCounts;
    friend class HVSubgraphCounts;

    bool recompute(
        bool arcAdd,
        boost::unordered_map<const Algora::Vertex *, int> &arcBorder) override;

    void vLVVertexChange(ILV *v, IncOrDecSingleKey *operation1,
                         IncOrDecSingleKey *operation2);
    void vLVArcChange(const Algora::Arc *a, IncOrDecSingleKey *operation,
                      bool recompute_after_removed);

    void uLvVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                         IncOrDecDoubleKey *operation2);
    void uLvArcChange(const Algora::Arc *a, IncOrDecDoubleKey *operation);

    void tVertexChange(ILV *v);
    void tArcChange(const Algora::Arc *a, IncOrDecSingleKey *operation);

    void uLLvVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                          IncOrDecDoubleKey *operation2);
    void uLLvArcChange(const Algora::Arc *a, IncOrDecDoubleKey *operation);

    void cLVVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                         IncOrDecDoubleKey *operation2);
    void cLVArcChange(const Algora::Arc *a, IncOrDecDoubleKey *operation,
                      bool recompute_after_removed);

    void pLLVertexChange(ILV *v, IncOrDecDoubleKey *operation1,
                         IncOrDecDoubleKey *operation2);
    void pLLArcChange(const Algora::Arc *a, IncOrDecDoubleKey *operation);

    void uHvVertexChange(ILV *v, IncOrDecDoubleKey *operation);
    void uHvArcChange(const Algora::Arc *a, IncOrDecDoubleKey *operation);

    void cLVertexChange(ILV *v, IncOrDecTripleKey *operation1,
                        IncOrDecTripleKey *operation2);
    void cLArcChange(const Algora::Arc *a, IncOrDecTripleKey *operation);

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

  public:
    bool initVertexPartition() override;
    void setObjectives(bool vLV = true, bool t = true, bool uLv = true,
                       bool uLLv = true, bool cLV = true, bool pLL = true,
                       bool uHv = true, bool cL = true);
    bool prepare() override;
    void cleanMaps() override;
    void cleanup() override;

    VertexPartition *getVertexPartition() { return vPart; }

    VertexPartition *setVertexPartition(VertexPartition *partition) {
        return vPart = partition;
    }

    // vLV Getter
    int getNrvLV(const Algora::Vertex *v) {
        auto it = vLV.find(v->getId());
        return (it == vLV.end() ? 0 : it->second);
    }
    auto vLV_begin() const { return vLV.begin(); }
    auto vLV_end() const { return vLV.end(); }

    // t Getter
    int getNrt(const Algora::Vertex *v) {
        auto it = t.find(v->getId());
        return (it == t.end() ? 0 : it->second);
    }
    auto t_begin() const { return t.begin(); }
    auto t_tend() const { return t.end(); }

    // uLv Getter
    int getNruLv(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = uLv.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = uLv.find({id1, id2});
        else
            it = uLv.find({id2, id1});

        return (it == uLv.end() ? 0 : it->second);
    }
    auto uLv_begin() const { return uLv.begin(); }
    auto uLv_end() const { return uLv.end(); }

    // uLLv Getter
    int getNruLLv(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = uLLv.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = uLLv.find({id1, id2});
        else
            it = uLLv.find({id2, id1});

        return (it == uLLv.end() ? 0 : it->second);
    }
    auto uLLv_begin() const { return uLLv.begin(); }
    auto uLLv_end() const { return uLLv.end(); }

    // cLV Getter
    int getNrcLV(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = cLV.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = cLV.find({id1, id2});
        else
            it = cLV.find({id2, id1});

        return (it == cLV.end() ? 0 : it->second);
    }
    int getNrcLV(const int id1, const int id2) {
        auto it = cLV.end();

        if (id1 < id2)
            it = cLV.find({id1, id2});
        else
            it = cLV.find({id2, id1});

        return (it == cLV.end() ? 0 : it->second);
    }
    auto cLV_begin() const { return cLV.begin(); }
    auto cLV_end() const { return cLV.end(); }

    // pLL Getter
    int getNrpLL(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = pLL.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = pLL.find({id1, id2});
        else
            it = pLL.find({id2, id1});

        return (it == pLL.end() ? 0 : it->second);
    }
    auto pLL_begin() const { return pLL.begin(); }
    auto pLL_end() const { return pLL.end(); }

    // uHv Getter
    int getNruHv(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = uHv.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = uHv.find({id1, id2});
        else
            it = uHv.find({id2, id1});

        return (it == uHv.end() ? 0 : it->second);
    }
    auto uHv_begin() const { return uHv.begin(); }
    auto uHv_end() const { return uHv.end(); }

    // cL Getter
    int getNrcL(const Algora::Vertex *v1, const Algora::Vertex *v2,
                const Algora::Vertex *v3) {
        auto it = cL.end();
        int id1 = v1->getId();
        int id2 = v2->getId();
        int id3 = v3->getId();

        if (id1 <= id2 && id2 <= id3)
            it = cL.find({id1, id2, id3});
        else if (id1 <= id3 && id3 <= id2)
            it = cL.find({id1, id3, id2});
        else if (id2 <= id1 && id1 <= id3)
            it = cL.find({id2, id1, id3});
        else if (id2 <= id3 && id3 <= id1)
            it = cL.find({id2, id3, id1});
        else if (id3 <= id1 && id1 <= id2)
            it = cL.find({id3, id1, id2});
        else if (id3 <= id2 && id2 <= id1)
            it = cL.find({id3, id2, id1});

        return (it == cL.end() ? 0 : it->second);
    }
    auto cL_begin() const { return cL.begin(); }
    auto cL_end() const { return cL.end(); }

    void printEpsTable() { vPart->print_table(); }

  protected:
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
};
