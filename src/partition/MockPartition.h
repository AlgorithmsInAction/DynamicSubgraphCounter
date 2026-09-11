#pragma once

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

class MockPartition : public VertexPartition {
  public:
    MockPartition(bool maintain_part_stats) {
        this->maintain_part_stats = maintain_part_stats;
    };
    ~MockPartition() override = default;

    bool prepare() override;
    void cleanup() override;
    void print_table() override;

    void EpsTabOnArcRemove(Algora::Arc *a) override;
    void EpsTabOnArcAdd(Algora::Arc *a) override;
    void EpsTabOnVertexRemove(ILV *v) override;

    int getTheoreticalMaxLowDegree() override { return graph->getSize(); };

    int getEpsilon() override {
        return 0;
        // std::cerr << "HIndex does not support Epsilon";
        // exit(0);
    }
    int getNumRecomputations() override {
        std::cerr << "MockPartition does not support Recomputations";
        exit(0);
    }
    void setOnRecompute(
        std::function<
            void(bool arcAdd,
                 boost::unordered_map<const Algora::Vertex *, int> &arcBorder)>
            on_recompute_func) override {}
    void removeOnRecompute() override {}

    std::string print_current_stats_header() {
        return "num_part_changes,H_size";
    };
    std::string print_current_stats() { return "0,0"; };

    std::string print_short_stats_header() override {
        return "noentry,noentry,noentry,noentry,noentry,noentry,noentry,"
               "noentry," +
               partChangeHeader();
        // noentry is only there to also have the same num of params as epstab
    };

    std::string print_short_stats() {
        return "0,0,0,0,0,0,0,0," + partChanges();
    };
};
