#ifndef E_SUBGRAPH_COUNTS_H
#define E_SUBGRAPH_COUNTS_H

#include "UndirectedFourSubgraphCounts.h"
#include "e_counts/EStructureCounts.h"

// #define edge_exist StrucCount.edge_exist
// #define is_low StrucCount.is_low
// #define is_high StrucCount.vPart->is_high_deg

class ESubgraphCounts : public UndirectedFourSubgraphCounts {
  private:
    EStructureCounts StrucCount;
    bool StrucCount_has_softVertexChange{0};
    bool StrucCount_has_s3_HighAnchors{0};

#if ENABLE_DEEPER_STATS
    std::chrono::duration<double> addTimeGraph{0};
    std::chrono::duration<double> delTimeGraph{0};

    size_t iterate_neighbor{0};
    size_t iterate_h{0};

    Algora::Arc *last_arc = nullptr;
#endif
#ifdef TRACK_CACHE
    MemoryCostCounter cacheMisses;
#endif

  public:
    ESubgraphCounts(Config &config) {
        this->objectives[0] = config.count_tCycles || config.count_paws;
        this->objectives[1] = config.count_tPaths;
        this->objectives[2] = config.count_claws;
        this->objectives[3] = config.count_paws;
        this->objectives[4] = config.count_fCycles;
        this->objectives[5] = config.count_diamonds;
        this->objectives[6] = config.count_fCliques;
        this->objectives[7] = config.g_counts;

        if (config.s_counts) {
            // error
            std::cerr
                << "S-count is not supported for Eppstein Subgraph Counting";
            exit(0);
        }

        StrucCount_has_softVertexChange = config.egst_direct;
        StrucCount.setSoftVertexChange(config.egst_direct);

        StrucCount_has_s3_HighAnchors = config.high_anchors_only;
        StrucCount.setHighAnchorsOnlyS3(config.high_anchors_only);
    }
    ~ESubgraphCounts() override = default;

    std::string getName() override { return "ESubgraphCounts"; }

    bool prepare() override;
    void unsetGraph() override;

    void tCycleArcChange(const Algora::Arc *a, CounterUpdate *operation);
    void DiamondArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void tPathArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void pawArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void fCycleArcChange(const Algora::Arc *v, CounterUpdate *operation);
    void fCliqueArcChange(const Algora::Arc *a, CounterUpdate *operation);
    void ClawArcChange(const Algora::Arc *a, CounterUpdate *operation);

    COUNTER_TYPE getNrTriangles(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
    }
    COUNTER_TYPE getNrTPaths(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
    }
    COUNTER_TYPE getNrClaws(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
    }
    COUNTER_TYPE getNrPaws(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
    }
    COUNTER_TYPE getNrfCycles(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
    }
    COUNTER_TYPE getNrDiamonds(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
    }
    COUNTER_TYPE getNrFCliques(ILV *v) override {
        std::cerr << "S-count is not supported for Eppstein Subgraph Counting";
        exit(0);
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
#ifdef TRACK_DEEP_TIME
                   "s3_MapInsert_Time,"
#endif
#if ENABLE_DEEPER_STATS
                   "op_s0,op_s1,op_s2,op_s3,op_s4,op_s5,op_s6,op_s7,s0_buckets,"
                   "s1_buckets,s2_buckets,s3_buckets,s4_buckets,s5_buckets,s6_"
                   "buckets,s7_buckets,iterate_neighbor,iterate_h,"
#endif
#ifdef TRACK_CACHE
                   "structure_add_del_cache_misses,structure_high_low_cache_"
                   "misses,subgraph_add_del_cache_misses,"
                   "structure_add_del_dtlb_misses,structure_high_low_dtlb_"
                   "misses,subgraph_add_del_dtlb_misses,"

#endif
                   ) +
               StrucCount.vPart->print_current_stats_header();
    }
    std::string print_current_stats() override {
        return std::to_string(getCurrentDynTime().count()) + std::string(",") +
#if ENABLE_DEEPER_STATS
#ifdef TRACK_DEEP_TIME
               std::to_string(StrucCount.s3_Time.count()) + "," +
#endif
               std::to_string(StrucCount.op_s0) + "," +
               std::to_string(StrucCount.op_s1) + "," +
               std::to_string(StrucCount.op_s2) + "," +
               std::to_string(StrucCount.op_s3) + "," +
               std::to_string(StrucCount.op_s4) + "," +
               std::to_string(StrucCount.op_s5) + "," +
               std::to_string(StrucCount.op_s6) + "," +
               std::to_string(StrucCount.op_s7) + "," +
               std::to_string(StrucCount.s0.bucket_count()) + "," +
               std::to_string(StrucCount.s1.bucket_count()) + "," +
               std::to_string(StrucCount.s2.bucket_count()) + "," +
               std::to_string(StrucCount.s3.bucket_count()) + "," +
               std::to_string(StrucCount.s4.bucket_count()) + "," +
               std::to_string(StrucCount.s5.bucket_count()) + "," +
               std::to_string(StrucCount.s6.bucket_count()) + "," +
               std::to_string(StrucCount.s7.bucket_count()) + "," +
               std::to_string(iterate_neighbor) + "," +
               std::to_string(iterate_h) + "," +
#endif
#ifdef TRACK_CACHE
               std::to_string(StrucCount.cacheMisses.getRAMAccesses()) + "," +
               std::to_string(StrucCount.cacheMissesVertex.getRAMAccesses()) +
               "," + std::to_string(cacheMisses.getRAMAccesses()) + "," +
               std::to_string(StrucCount.cacheMisses.getDTLBMisses()) + "," +
               std::to_string(StrucCount.cacheMissesVertex.getDTLBMisses()) +
               "," + std::to_string(cacheMisses.getDTLBMisses()) + "," +

#endif
               StrucCount.vPart->print_current_stats();
    }

    std::string print_short_stats_header() override {
        return "directVertexChange,s3HighAnchors,"
#if ENABLE_DEEPER_STATS
               "structure_addTime,structure_delTime,structure_"
               "toHighTime,structure_toLowTime,subgraph_addTime,subgraph_"
               "delTime,op_s0,op_s1,op_s2,op_s3,op_s4,op_s5,op_s6,op_s7,"
               "iterate_neighbor,iterate_h,"
#endif
               + StrucCount.vPart->print_short_stats_header();
    };

    std::string print_short_stats() override {
        return std::to_string(StrucCount_has_softVertexChange) + "," +
               std::to_string(StrucCount_has_s3_HighAnchors) + "," +
#if ENABLE_DEEPER_STATS
               std::to_string(StrucCount.addTime.count()) + "," +
               std::to_string(StrucCount.delTime.count()) + "," +
               std::to_string(StrucCount.toHighTime.count()) + "," +
               std::to_string(StrucCount.toLowTime.count()) + "," +
               std::to_string(addTimeGraph.count()) + "," +
               std::to_string(delTimeGraph.count()) + "," +
               std::to_string(StrucCount.op_s0) + "," +
               std::to_string(StrucCount.op_s1) + "," +
               std::to_string(StrucCount.op_s2) + "," +
               std::to_string(StrucCount.op_s3) + "," +
               std::to_string(StrucCount.op_s4) + "," +
               std::to_string(StrucCount.op_s5) + "," +
               std::to_string(StrucCount.op_s6) + "," +
               std::to_string(StrucCount.op_s7) + "," +
               std::to_string(iterate_neighbor) + "," +
               std::to_string(iterate_h) + "," +
#endif
               StrucCount.vPart->print_short_stats();
    }

    void write_debug() override {
        std::cout << "Table: ";

        StrucCount.vPart->print_table();

        std::cout << "\n";

        // Inline function to print SingleKeyMap
        auto printSingleKeyMap = [](const SingleKeyMap &map,
                                    const std::string &label) {
            std::cout << label << ": ";
            bool first = true; // To handle commas
            for (const auto &entry : map) {
                if (!first) {
                    std::cout << ", ";
                }
                std::cout << "'" << entry.first << "' : " << entry.second;
                first = false;
            }
            std::cout << std::endl;
        };

        // Inline function to print DoubleKeyMap
        auto printDoubleKeyMap = [](const DoubleKeyMap &map,
                                    const std::string &label) {
            std::cout << label << ": ";
            bool first = true; // To handle commas
            for (const auto &entry : map) {
                if (!first) {
                    std::cout << ", ";
                }
                std::cout << "'" << entry.first.first << ", "
                          << entry.first.second << "' : " << entry.second;
                first = false;
            }
            std::cout << std::endl;
        };

        // Inline function to print TripleKeyMap
        auto printTripleKeyMap = [](const TripleKeyMap &map,
                                    const std::string &label) {
            std::cout << label << ": ";
            bool first = true; // To handle commas
            for (const auto &entry : map) {
                if (!first) {
                    std::cout << ", ";
                }
                std::cout << "'" << std::get<0>(entry.first) << ", "
                          << std::get<1>(entry.first) << ", "
                          << std::get<2>(entry.first) << "' : " << entry.second;
                first = false;
            }
            std::cout << std::endl;
        };

        // Print SingleKeyMaps
        printSingleKeyMap(StrucCount.s0, "s0");
        printSingleKeyMap(StrucCount.s1, "s1");

        // Print DoubleKeyMaps
        printDoubleKeyMap(StrucCount.s2, "s2");
        printDoubleKeyMap(StrucCount.s3, "s3");
        printDoubleKeyMap(StrucCount.s4, "s4");
        printDoubleKeyMap(StrucCount.s5, "s5");
        printDoubleKeyMap(StrucCount.s6, "s6");

        // Print TripleKeyMap
        printTripleKeyMap(StrucCount.s7, "s7");

        std::cout << " ------------------------------------------ "
                  << std::endl;
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