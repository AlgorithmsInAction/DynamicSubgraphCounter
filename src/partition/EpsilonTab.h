#pragma once

#include "config.h"
#include "definitions.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "graph/digraph.h"
#include "graph/graph_functional.h"
#include "graph/graphartifact.h"
#include "graph/vertex.h"
#include "observable.h"
#include "partition/VertexPartition.h"
#include "property/fastpropertymap.h"
#include <boost/container_hash/extensions.hpp>
#include <boost/container_hash/hash_fwd.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <chrono>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

class EpsilonTab : public VertexPartition {

    double epsilon{-1};

    std::chrono::duration<double> totalRecomputationTime{0};

    std::function<void(bool arcAdd, boost::unordered_map<const Algora::Vertex *,
                                                         int> &arcBorder)>
        on_recompute_func = nullptr;

    int num_recomputations_last_step{0};
    int num_recomputations{0};
    double recomputeFactor{2};
    bool soft_recompute{0};
    bool lazy_recompute{0};
    int round_robin_size{0};
    double round_robin_percent{0};
    u_int32_t round_robin_id{0};

    bool notify_before_change{0};

    bool delayed_recompute{0};
    std::deque<ILV *> wronglyPartitioned;
    int adjust_per_update;

    bool recompute(Algora::Arc *a, bool arcAdd);
    bool check_recompute(Algora::Arc *a, bool arcAdd);
    void round_robin();

  public:
    EpsilonTab(Config &config) {

        epsilon = config.epsilon;
        recomputeFactor = config.recomputeFactor;
        soft_recompute = config.soft_recompute;
        lazy_recompute = config.lazy_recompute;
        delayed_recompute = config.delayed_recompute;
        round_robin_size = config.round_robin_size;
        round_robin_percent = config.round_robin_percent;
        notify_before_change = config.epstab_notify_before_change;
        maintain_part_stats = config.maintain_part_stats;

        if (round_robin_size && round_robin_percent) {
            std::cerr << "Can not set both round_robin_size and "
                         "round_robin_percent\n";
            exit(0);
        }
        if ((epsilon <= 0) || (epsilon >= 1)) {
            std::cerr << "Epsilon must be between 0 and 1\n";
            exit(0);
        }
    };
    ~EpsilonTab() override = default;

    using VertexPartition::is_high_deg;

    bool setEpsilon(const double epsilon);

    bool prepare() override;
    void cleanup() override;
    void print_table() override;

    void EpsTabOnArcRemove(Algora::Arc *a) override;
    void EpsTabOnArcAdd(Algora::Arc *a) override;
    void EpsTabOnVertexRemove(ILV *v) override;

    std::chrono::duration<double> getTotalRecomputationTime() override {
        return totalRecomputationTime;
    }

    std::string print_current_stats_header() { return "recomputation,H_size"; };
    std::string print_current_stats() {
        if (num_recomputations != num_recomputations_last_step) {
            num_recomputations_last_step = num_recomputations;
            return std::to_string(1) + "," + std::to_string(HighDeg.size());
        } else {
            return std::to_string(0) + "," + std::to_string(HighDeg.size());
        }
    };

    std::string print_short_stats_header() override {
        return "epsilon,recompute_factor,recompute_mode,rr_percent,rr_size,"
               "notify_early,num_recomputations,recompute_time," +
               partChangeHeader();
    };
    std::string print_short_stats() {
        return std::to_string(epsilon) + "," + std::to_string(recomputeFactor) +
               "," +
               std::to_string(
                   soft_recompute
                       ? 1
                       : (lazy_recompute ? 2 : (delayed_recompute ? 3 : 0))) +
               "," + std::to_string(round_robin_percent) + "," +
               std::to_string(round_robin_size) + "," +
               std::to_string(notify_before_change) + "," +
               std::to_string(num_recomputations) + "," +
               std::to_string(totalRecomputationTime.count()) + "," +
               partChanges();
    };

    /**
     * @brief Set the On Recompute object, which will be called
     * in case of a recomputation after the Epsilon Partition was
     * recalculated.
     *
     * @param on_recompute_func
     */
    void setOnRecompute(
        std::function<
            void(bool arcAdd,
                 boost::unordered_map<const Algora::Vertex *, int> &arcBorder)>
            on_recompute_func) override {
        this->on_recompute_func = on_recompute_func;
    }
    void removeOnRecompute() override { this->on_recompute_func = nullptr; }

    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getHighDegBegin(ILV *v) {
        return v->getEdges().begin();
    }
    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getHighDegEnd(ILV *v) {
        return v->getEdges().begin() + arcBorder[v];
    }
    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getLowDegBegin(ILV *v) {
        return v->getEdges().begin() + arcBorder[v];
    }
    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getLowDegEnd(ILV *v) {
        return v->getEdges().end();
    }

    int getTheoreticalMaxLowDegree() override { return 1.5 * theta; };

    int getEpsilon() override { return epsilon; }

    int getNumRecomputations() override { return num_recomputations; }
};
