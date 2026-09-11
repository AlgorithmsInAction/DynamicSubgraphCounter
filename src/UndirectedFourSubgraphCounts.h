#ifndef U4_SUBGRAPH_COUNTS_H
#define U4_SUBGRAPH_COUNTS_H

#include "SubgraphCounts.h"
#include "definitions.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/vertex.h"
#include "partition/VertexPartition.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <chrono>
#include <string>
#include <vector>

class UndirectedFourSubgraphCounts : public SubgraphCounts {

  public:
    UndirectedFourSubgraphCounts() {}
    virtual ~UndirectedFourSubgraphCounts() = default;

    virtual std::string getName() { return "SubgraphCounts"; }

    COUNTER_TYPE getNrtCycle() override { return tCycle; }
    COUNTER_TYPE getNrDiamonds() { return diamond; }
    COUNTER_TYPE getNrTPaths() { return tPath; }
    COUNTER_TYPE getNrPaws() { return paw; }
    COUNTER_TYPE getNrfCycles() { return fCycle; }
    COUNTER_TYPE getNrClaws() { return claw; }
    COUNTER_TYPE getNrFCliques() { return fClique; }

    virtual COUNTER_TYPE getNrTriangles(ILV *v) = 0;
    virtual COUNTER_TYPE getNrTPaths(ILV *v) = 0;
    virtual COUNTER_TYPE getNrClaws(ILV *v) = 0;
    virtual COUNTER_TYPE getNrPaws(ILV *v) = 0;
    virtual COUNTER_TYPE getNrfCycles(ILV *v) = 0;
    virtual COUNTER_TYPE getNrDiamonds(ILV *v) = 0;
    virtual COUNTER_TYPE getNrFCliques(ILV *v) = 0;

    std::string print_global_count_header() override {

        return "#triangles,#diamonds,#threePaths,#fourCycles,#claws,#"
               "fourCliques,#paw";
    }

    virtual std::string print_current_stats_header() { return "time"; }
    virtual std::string print_current_stats() {
        return std::to_string(getCurrentDynTime().count());
    }
    virtual std::string print_short_stats_header() { return ""; }
    virtual std::string print_short_stats() { return ""; }

    std::string print_current_output_global_count() override {

        return std::to_string(getNrtCycle()) + "," +
               std::to_string(getNrDiamonds()) + "," +
               std::to_string(getNrTPaths()) + "," +
               std::to_string(getNrfCycles()) + "," +
               std::to_string(getNrClaws()) + "," +
               std::to_string(getNrFCliques()) + "," +
               std::to_string(getNrPaws());
    }

  protected:
    COUNTER_TYPE diamond{0};
    COUNTER_TYPE tPath{0};
    COUNTER_TYPE paw{0};
    COUNTER_TYPE fCycle{0};
    COUNTER_TYPE fClique{0};
    COUNTER_TYPE claw{0};
    COUNTER_TYPE tCycle{0};

    bool objectives[9]{false};
    bool all_s{false};
};

#endif