#ifndef HV_SUBGRAPH_COUNTS_H
#define HV_SUBGRAPH_COUNTS_H

#include "UndirectedFourSubgraphCounts.h"
#include "h_vanilla/HVStructureCounts.h"

class HVSubgraphCounts : public UndirectedFourSubgraphCounts {
  private:
    HVStructureCounts StrucCount;
    SingleKeyMap sTriangle;
    SingleKeyMap sTPath;
    SingleKeyMap sClaw;
    SingleKeyMap sPaw;
    SingleKeyMap sFCycle;
    SingleKeyMap sDiamond;
    SingleKeyMap sFClique;

#if ENABLE_DEEPER_STATS
    std::chrono::duration<double> addTimeGraph{0};
    std::chrono::duration<double> delTimeGraph{0};

    size_t iterate_neighbor{0};
    size_t iterate_h{0};
#endif

    boost::unordered_flat_map<ILV *, std::monostate> CurrentObservedVertices;
    boost::unordered_flat_map<int, std::monostate> observedVertices;
    boost::unordered_flat_map<int, std::monostate> already_observed;

  public:
    HVSubgraphCounts(Config &config) {
        for (auto v : config.v_list) {
            this->observedVertices.emplace(v, std::monostate{});
        }
        if (observedVertices.size() == 0)
            all_s = true;

        this->objectives[0] = config.count_tCycles || config.count_paws;
        this->objectives[1] = config.count_tPaths;
        this->objectives[2] = config.count_claws;
        this->objectives[3] = config.count_paws;
        this->objectives[4] = config.count_fCycles;
        this->objectives[5] = config.count_diamonds;
        this->objectives[6] = config.count_fCliques;
        this->objectives[7] = config.g_counts;
        this->objectives[8] = config.s_counts;
    };
    ~HVSubgraphCounts() override = default;

    std::string getName() override { return "HVSubgraphCounts"; }

    bool prepare() override;
    void unsetGraph() override;

    void tCycleArcChange(const Algora::Arc *a, CounterUpdate *operation);
    void DiamondArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void tPathArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void pawArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void fCycleArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void fCliqueArcChange(const Algora::Arc *a, CounterUpdate *operation);
    void ClawArcChange(const Algora::Arc *a, CounterUpdate *operation);

    COUNTER_TYPE getNrTriangle(ILV *v);

    COUNTER_TYPE getNrTPathsWithEdge(ILV *u, ILV *v);
    COUNTER_TYPE getNrTriangleWithEdge(ILV *u, ILV *v);
    COUNTER_TYPE getNrTriangleWithEdge_pretended(ILV *u, ILV *v, ILV *e1,
                                                 ILV *e2);
    COUNTER_TYPE getNrTriangle_pretended(ILV *v, ILV *e1, ILV *e2);
    COUNTER_TYPE getNrPawsWithEdge(ILV *head, ILV *tail, COUNTER_TYPE tHead,
                                   COUNTER_TYPE tTail, COUNTER_TYPE tHeadTail);
    COUNTER_TYPE getNrfCyclesWithEdge(ILV *u, ILV *v);
    COUNTER_TYPE getNrClawsWithEdge(ILV *head, ILV *tail);
    COUNTER_TYPE getNrDiamondsWithEdge(ILV *u, ILV *v);
    COUNTER_TYPE getNrFCliquesWithEdge(ILV *u, ILV *v);

    COUNTER_TYPE getNrTriangles(ILV *v) override {
        auto it = sTriangle.find(v->getId());
        return (it == sTriangle.end() ? 0 : it->second);
    }
    COUNTER_TYPE getNrTPaths(ILV *v) override {
        auto it = sTPath.find(v->getId());
        return (it == sTPath.end() ? 0 : it->second);
    }
    COUNTER_TYPE getNrClaws(ILV *v) override {
        auto it = sClaw.find(v->getId());
        return (it == sClaw.end() ? 0 : it->second);
    }
    COUNTER_TYPE getNrPaws(ILV *v) override {
        auto it = sPaw.find(v->getId());
        return (it == sPaw.end() ? 0 : it->second);
    }
    COUNTER_TYPE getNrfCycles(ILV *v) override {
        auto it = sFCycle.find(v->getId());
        return (it == sFCycle.end() ? 0 : it->second);
    }
    COUNTER_TYPE getNrDiamonds(ILV *v) override {
        auto it = sDiamond.find(v->getId());
        return (it == sDiamond.end() ? 0 : it->second);
    }
    COUNTER_TYPE getNrFCliques(ILV *v) override {
        auto it = sFClique.find(v->getId());
        return (it == sFClique.end() ? 0 : it->second);
    }

    void setVertexPartition(VertexPartition *vPart) {
        StrucCount.setVertexPartition(vPart);
    };

    std::chrono::duration<double> getCurrentDynTime() override {
        return StrucCount.vPart->getCurrentDynTime();
    }

    inline bool edge_exist(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        return StrucCount.edge_exist(v1, v2);
    }
    inline bool is_low(const Algora::Vertex *v) { return StrucCount.is_low(v); }
    inline bool is_high(const Algora::Vertex *v) {
        return StrucCount.is_high(v);
    }

    std::string print_current_stats_header() override {
        return std::string(
                   "time,"
#if ENABLE_DEEPER_STATS
                   "op_vLV,op_uLv,op_t,op_uLLv,op_cLV,op_pLL,op_uHv,op_cL,"
                   "vLV_buckets,uLv_buckets,t_buckets,uLLv_buckets,cLV_buckets,"
                   "pLL_buckets,uHv_buckets,cL_buckets,iterate_neighbor,"
                   "iterate_h,"
#endif
                   ) +
               StrucCount.vPart->print_current_stats_header();
    }
    std::string print_current_stats() override {
        return std::to_string(getCurrentDynTime().count()) + std::string(",") +
#if ENABLE_DEEPER_STATS
               std::to_string(StrucCount.op_vLV) + "," +
               std::to_string(StrucCount.op_uLv) + "," +
               std::to_string(StrucCount.op_t) + "," +
               std::to_string(StrucCount.op_uLLv) + "," +
               std::to_string(StrucCount.op_cLV) + "," +
               std::to_string(StrucCount.op_pLL) + "," +
               std::to_string(StrucCount.op_uHv) + "," +
               std::to_string(StrucCount.op_cL) + "," +
               std::to_string(StrucCount.vLV.bucket_count()) + "," +
               std::to_string(StrucCount.uLv.bucket_count()) + "," +
               std::to_string(StrucCount.t.bucket_count()) + "," +
               std::to_string(StrucCount.uLLv.bucket_count()) + "," +
               std::to_string(StrucCount.cLV.bucket_count()) + "," +
               std::to_string(StrucCount.pLL.bucket_count()) + "," +
               std::to_string(StrucCount.uHv.bucket_count()) + "," +
               std::to_string(StrucCount.cL.bucket_count()) + "," +
               std::to_string(iterate_neighbor) + "," +
               std::to_string(iterate_h) + "," +
#endif
               StrucCount.vPart->print_current_stats();
    }

    std::string print_short_stats_header() override {
        // noentry only for symmetry reasons with egst
        return "use_all_aux,aux_for_t,"
#if ENABLE_DEEPER_STATS
               "structure_addTime,structure_delTime,structure_"
               "toHighTime,structure_toLowTime,subgraph_addTime,subgraph_"
               "delTime,op_vLV,op_uLv,op_t,op_uLLv,op_cLV,op_pLL,op_uHv,op_cL,"
               "iterate_neighbor,iterate_h,"
#endif
               + StrucCount.vPart->print_short_stats_header();
    };

    std::string print_short_stats() override {
        return "0,1," +
#if ENABLE_DEEPER_STATS
               std::to_string(StrucCount.addTime.count()) + "," +
               std::to_string(StrucCount.delTime.count()) + "," +
               std::to_string(StrucCount.toHighTime.count()) + "," +
               std::to_string(StrucCount.toLowTime.count()) + "," +
               std::to_string(addTimeGraph.count()) + "," +
               std::to_string(delTimeGraph.count()) + "," +
               std::to_string(StrucCount.op_vLV) + "," +
               std::to_string(StrucCount.op_uLv) + "," +
               std::to_string(StrucCount.op_t) + "," +
               std::to_string(StrucCount.op_uLLv) + "," +
               std::to_string(StrucCount.op_cLV) + "," +
               std::to_string(StrucCount.op_pLL) + "," +
               std::to_string(StrucCount.op_uHv) + "," +
               std::to_string(StrucCount.op_cL) + "," +
               std::to_string(iterate_neighbor) + "," +
               std::to_string(iterate_h) + "," +
#endif

               StrucCount.vPart->print_short_stats();
    }

    void write_debug() override {
        StrucCount.vPart->print_table();

        std::cout << "\n----vLV----\n";
        for (auto it = StrucCount.vLV_begin(); it != StrucCount.vLV_end(); it++)
            std::cout << "Vertex " << it->first << " : " << it->second << "\n";

        std::cout << "\n----uLv----\n";
        for (auto it = StrucCount.uLv_begin(); it != StrucCount.uLv_end(); it++)
            std::cout << "Vertex Pair " << it->first.first << "-"
                      << it->first.second << " : " << it->second << "\n";

        std::cout << "\n----t----\n";
        for (auto it = StrucCount.t_begin(); it != StrucCount.t_tend(); it++)
            std::cout << "Vertex " << it->first << " : " << it->second << "\n";

        std::cout << "\n----uLLv----\n";
        for (auto it = StrucCount.uLLv_begin(); it != StrucCount.uLLv_end();
             it++)
            std::cout << "Vertex Pair " << it->first.first << "-"
                      << it->first.second << " : " << it->second << "\n";

        std::cout << "\n----cLV----\n";
        for (auto it = StrucCount.cLV_begin(); it != StrucCount.cLV_end(); it++)
            std::cout << "Vertex Pair " << it->first.first << "-"
                      << it->first.second << " : " << it->second << "\n";

        std::cout << "\n----pLL----\n";
        for (auto it = StrucCount.pLL_begin(); it != StrucCount.pLL_end(); it++)
            std::cout << "Vertex Pair " << it->first.first << "-"
                      << it->first.second << " : " << it->second << "\n";

        std::cout << "\n----uHv----\n";
        for (auto it = StrucCount.uHv_begin(); it != StrucCount.uHv_end(); it++)
            std::cout << "Vertex Pair " << it->first.first << "-"
                      << it->first.second << " : " << it->second << "\n";

        std::cout << "\n----cL----\n";
        for (auto it = StrucCount.cL_begin(); it != StrucCount.cL_end(); it++)
            std::cout << "Vertex Pair " << std::get<0>(it->first) << "-"
                      << std::get<1>(it->first) << "-" << std::get<2>(it->first)
                      << " : " << it->second << "\n";
    }

    template <typename Func>
    void
    forEachTwoHighNeighbors(ILV *v,
                            const std::vector<const ILV *> &excludeVertices,
                            Func operation) {
        StrucCount.forEachTwoHighNeighbors(v, excludeVertices, operation);
    }

    template <typename Func>
    void forEachHighNeighbor(ILV *v,
                             const std::vector<const ILV *> &excludeVertices,
                             ILV *vStop, Func operation) {
        StrucCount.forEachHighNeighbor(v, excludeVertices, vStop, operation);
    }

    template <typename Func>
    void forEachLowNeighbor(ILV *v,
                            const std::vector<const ILV *> &excludeVertices,
                            ILV *vStop, Func operation) {
        StrucCount.forEachLowNeighbor(v, excludeVertices, vStop, operation);
    }

    template <typename Func>
    void forEachNeighbor(ILV *v,
                         const std::vector<const ILV *> &excludeVertices,
                         Func operation) {
        StrucCount.forEachNeighbor(v, excludeVertices, operation);
    }

    template <typename Func>
    void forEachNeighbor(ILV *v,
                         const std::vector<const ILV *> &excludeVertices,
                         ILV *vStop, Func operation) {
        StrucCount.forEachNeighbor(v, excludeVertices, vStop, operation);
    }
};

#endif