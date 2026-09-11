#pragma once

#include "ObserverAlgorithm.h"
#include "SubgraphCounts.h"
#include "UndirectedFourSubgraphCounts.h"
#include "definitions.h"
#include "partition/VertexPartition.h"
#include "streaming_stats.h"
#include <boost/math/special_functions/binomial.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <variant>

// #define READABLE_STATS

class GraphStats : public ObserverAlgorithm {
  public:
    GraphStats() = default;
    virtual ~GraphStats() = default;

    bool prepare() override {
        graph->onArcAdd(&OnArcAddId.emplace_back(1), [&](Algora::Arc *a) {
            if (vPart->fixed) {
                return;
            }
            ILV *head = CAST_ILV(a->getHead());
            ILV *tail = CAST_ILV(a->getTail());

            num_v += !(nodes.contains(head->getId())) +
                     !(nodes.contains(tail->getId()));

            ++num_e;

            nodes.emplace(head->getId(), std::monostate{});
            nodes.emplace(tail->getId(), std::monostate{});

            sum_deg += 2;

            auto deg_h = graph->getUndirectedDegree(head);
            auto deg_t = graph->getUndirectedDegree(tail);

            if (deg_h > max_deg) {
                max_deg = deg_h;
                max_deg_time = time;
            }

            if (deg_t > max_deg) {
                max_deg = deg_t;
                max_deg_time = time;
            }

            num_triplets += deg_h - 1;
            num_triplets += deg_t - 1;

            avg_deg_stats.add(num_v ? sum_deg / num_v : 0);
            cc_stats.add(num_triplets
                             ? ((double)SubCounts->getNrtCycle()) / num_triplets
                             : 0);
            node_stats.add(num_v);
            edge_stats.add(num_e);
            h_stats.add(vPart->get_num_high_deg());
            if (num_e > 0) {
                h2_m_stats.add(std::pow(vPart->get_num_high_deg(), 2) /
                               (double)num_e);
                h3_m_stats.add(std::pow(vPart->get_num_high_deg(), 3) /
                               (double)num_e);
            } else {
                h2_m_stats.add(0);
                h3_m_stats.add(0);
            }

            ++time;
        });

        OnArcRemoveId.reserve(1);
        graph->onArcRemove(&OnArcRemoveId.emplace_back(1), [&](Algora::Arc *a) {
            if (vPart->fixed) {
                return;
            }
            ILV *head = CAST_ILV(a->getHead());
            ILV *tail = CAST_ILV(a->getTail());
            sum_deg -= 2;
            --num_e;

            auto deg_h = graph->getUndirectedDegree(head);
            auto deg_t = graph->getUndirectedDegree(tail);

            if (deg_h - 1 <= 0) {
                nodes.erase(head->getId());
                --num_v;
            }
            if (deg_t - 1 <= 0) {
                nodes.erase(tail->getId());
                --num_v;
            }

            num_triplets -= deg_h - 1;
            num_triplets -= deg_t - 1;

            avg_deg_stats.add(num_v ? sum_deg / num_v : 0);
            cc_stats.add(num_triplets
                             ? ((double)SubCounts->getNrtCycle()) / num_triplets
                             : 0);
            node_stats.add(num_v);
            edge_stats.add(num_e);
            h_stats.add(vPart->get_num_high_deg());
            if (num_e > 0) {
                h2_m_stats.add(std::pow(vPart->get_num_high_deg(), 2) /
                               (double)num_e);
                h3_m_stats.add(std::pow(vPart->get_num_high_deg(), 3) /
                               (double)num_e);
            } else {
                h2_m_stats.add(0);
                h3_m_stats.add(0);
            }
            ++time;
        });

        return true;
    }

    void setSubgraphCounts(SubgraphCounts *SubCounts, VertexPartition *part) {
        this->SubCounts = SubCounts;
        vPart = part;
    }

    void setConfig(Config &config) {

        auto graph = config.input_graph_file;

        size_t posEnd = graph.rfind('.');   // find the last dot
        size_t posStart = graph.rfind('/'); // find the last dot
        if (posEnd != std::string::npos) {
            graph = graph.substr(0, posEnd);
        }
        if (posStart != std::string::npos) {
            graph = graph.substr(posStart + 1);
        }
        graph_name = graph;
    }

    void printStats() {

#ifdef READABLE_STATS
        std::cout << std::fixed << std::setprecision(4);

        std::cout << "=== Graph Statistics ===\n"
                  << "Number of nodes mean      : " << node_stats.mean() << "\n"
                  << "Number of nodes max       : " << node_stats.max() << "\n"
                  << "Number of nodes q99       : " << node_stats.quantile(0.99)
                  << "\n"
                  << "Number of edges q1        : " << edge_stats.quantile(0.01)
                  << "Number of edges mean      : " << edge_stats.mean() << "\n"
                  << "Number of edges max       : " << edge_stats.max() << "\n"
                  << "Number of edges q99       : " << edge_stats.quantile(0.99)
                  << "\n"
                  << "Number of nodes q1        : " << node_stats.quantile(0.01)
                  << "\n"
                  << "Edge insertions      : " << vPart->edge_insertions << "\n"
                  << "Edge removals        : " << vPart->edge_removals << "\n"
                  << "Max degree           : " << max_deg
                  << " (at t = " << max_deg_time << ")\n"
                  << "Avg degree mean      : " << avg_deg_stats.mean() << "\n"
                  << "Avg degree max       : " << avg_deg_stats.max() << "\n"
                  << "Avg degree q99       : " << avg_deg_stats.quantile(0.99)
                  << "\n"
                  << "Avg degree q1        : " << avg_deg_stats.quantile(0.01)
                  << "\n"
                  << "Clustering coefficient mean      : " << cc_stats.mean()
                  << "\n"
                  << "Clustering coefficient max       : " << cc_stats.max()
                  << "\n"
                  << "Clustering coefficient q99       : "
                  << cc_stats.quantile(0.99) << "\n"
                  << "Clustering coefficient q1        : "
                  << cc_stats.quantile(0.01) << "\n"
                  << "============================\n";

#else
        std::cout << graph_name << "," << time << "," << vPart->edge_insertions
                  << "," << vPart->edge_removals << "," << node_stats.mean()
                  << "," << node_stats.max() << "," << node_stats.quantile(0.99)
                  << "," << node_stats.quantile(0.01) << ","
                  << edge_stats.mean() << "," << edge_stats.max() << ","
                  << edge_stats.quantile(0.99) << ","
                  << edge_stats.quantile(0.01) << "," << max_deg << ","
                  << max_deg_time << "," << avg_deg_stats.mean() << ","
                  << avg_deg_stats.max() << "," << avg_deg_stats.quantile(0.99)
                  << "," << avg_deg_stats.quantile(0.01) << ","
                  << h_stats.mean() << "," << h_stats.max() << ","
                  << h_stats.quantile(0.99) << "," << h_stats.quantile(0.01)
                  << "," << h2_m_stats.mean() << "," << h2_m_stats.max() << ","
                  << h2_m_stats.quantile(0.99) << ","
                  << h2_m_stats.quantile(0.01) << "," << h3_m_stats.mean()
                  << "," << h3_m_stats.max() << "," << h3_m_stats.quantile(0.99)
                  << "," << h3_m_stats.quantile(0.01) << "," << cc_stats.mean()
                  << "," << cc_stats.max() << "," << cc_stats.quantile(0.99)
                  << "," << cc_stats.quantile(0.01) << "\n";
#endif
    }

  private:
    std::string graph_name;

    SubgraphCounts *SubCounts;
    VertexPartition *vPart;
    COUNTER_TYPE time = 0;
    COUNTER_TYPE sum_deg = 0;
    COUNTER_TYPE num_v = 0;
    COUNTER_TYPE num_e = 0;
    COUNTER_TYPE num_triplets = 0;

    boost::unordered_flat_map<int, std::monostate> nodes;

    COUNTER_TYPE max_deg{0};
    COUNTER_TYPE max_deg_time{0};
    StreamingStats edge_stats;
    StreamingStats node_stats;
    StreamingStats avg_deg_stats;
    StreamingStats cc_stats;
    StreamingStats h_stats;
    StreamingStats h2_m_stats;
    StreamingStats h3_m_stats;
};