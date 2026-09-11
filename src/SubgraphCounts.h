#ifndef SUBGRAPH_COUNTS_H
#define SUBGRAPH_COUNTS_H

#include "config.h"
#include "definitions.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/vertex.h"
#include "Graph.h"
#include "QuadCensus.h"
#include "partition/VertexPartition.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <chrono>
#include <string>
#include <vector>

class SubgraphCounts {

  public:
    Algora::IncidenceListGraph *graph;

    virtual std::string getName() { return "SubgraphCounts"; }

    static void add(COUNTER_TYPE &counter, COUNTER_TYPE increment) {
        if (increment <= 0) {
            return;
        }
        counter += increment;
    }
    static void subtract(COUNTER_TYPE &counter, COUNTER_TYPE decrement) {
        if (decrement <= 0) {
            return;
        }
        if (decrement >= counter) {
            counter = 0;
            return;
        }
        counter -= decrement;
    }

  public:
    virtual bool prepare() = 0;

    SubgraphCounts() : graph(nullptr) {}
    virtual ~SubgraphCounts() = default;
    void setGraph(Algora::IncidenceListGraph *graph) { this->graph = graph; }
    virtual void unsetGraph() { this->graph = nullptr; }
    bool hasGraph() { return graph != nullptr; };

    virtual std::chrono::duration<double> getCurrentDynTime() = 0;

    virtual void setVertexPartition(VertexPartition *vPart) = 0;
    virtual void write_debug() = 0;
    virtual std::string print_global_count_header() = 0;
    virtual std::string print_current_output_global_count() = 0;

    virtual std::string print_current_stats_header() = 0;
    virtual std::string print_current_stats() = 0;

    virtual std::string print_short_stats_header() = 0;
    virtual std::string print_short_stats() = 0;

    virtual COUNTER_TYPE getNrtCycle() = 0;
};

#endif
