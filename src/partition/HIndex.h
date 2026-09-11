#pragma once

#include "config.h"
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
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class HIndex : public VertexPartition {

    // These are on the bubble vertices: if the h-index is to be reduced by one,
    // each of these elements must be removed from HelperH (originally B)
    boost::unordered_flat_map<int, Algora::Vertex *> Bubble;
    // Vertices by degree classes (originally C)
    std::vector<boost::unordered_flat_map<int, Algora::Vertex *>> VerticesOfDeg;
    // Helper set H. This is the directly based on the h-index. Note that only
    // vertices with deg>=2*|HelperH| are considered high degree
    boost::unordered_flat_map<int, Algora::Vertex *> HelperH;

    double gradual_factor;

  public:
    HIndex(Config &config) {
        this->maintain_part_stats = config.maintain_part_stats;
        this->gradual_factor = config.hind_gradual_factor;
    };
    ~HIndex() override = default;

    using VertexPartition::is_high_deg;

    bool prepare() override;
    void cleanup() override;
    void print_table() override;

    void EpsTabOnArcRemove(Algora::Arc *a) override;
    void EpsTabOnArcAdd(Algora::Arc *a) override;
    void EpsTabOnVertexRemove(ILV *v) override;

    void EpsTabOnVertexAdd(Algora::Vertex *v);
    void VertexRemove(Algora::Vertex *v, int deg);
    void VertexAdd(Algora::Vertex *v, int deg);
    void AdjustDegOfVertexInH(Algora::Vertex *v, int deg1, int deg2);

    void setOnRecompute(
        std::function<
            void(bool arcAdd,
                 boost::unordered_map<const Algora::Vertex *, int> &arcBorder)>
            on_recompute_func) override {}
    void removeOnRecompute() override {}

    int getTheoreticalMaxLowDegree() override { return HighDeg.size(); };

    int getEpsilon() override {
        return 0;
        // std::cerr << "HIndex does not support Epsilon";
        // exit(0);
    }
    int getNumRecomputations() override {
        std::cerr << "HIndex does not support Recomputations";
        exit(0);
    }

    std::string print_current_stats_header() {
        return "num_part_changes,H_size";
    };
    std::string print_current_stats() {
        return std::to_string(num_part_changes_per_step) + "," +
               std::to_string(HighDeg.size());
    };

    std::string print_short_stats_header() override {
        return "gradual_factor,noentry,noentry,noentry,noentry,noentry,noentry,"
               "noentry," +
               partChangeHeader();
        // noentry is only there to also have the same num of params as epstab
    };

    std::string print_short_stats() {
        return std::to_string(gradual_factor) + ",0,0,0,0,0,0,0," +
               partChanges();
    };
};
