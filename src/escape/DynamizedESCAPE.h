#ifndef ESCAPE_SUBGRAPH_COUNTS_H
#define ESCAPE_SUBGRAPH_COUNTS_H

#include "ObserverAlgorithm.h"
#include "UndirectedFourSubgraphCounts.h"
#include "static/StaticAlgorithm.h"
#include <algorithm/staticalgorithmwrapper.h>

class DynamizedESCAPE : public UndirectedFourSubgraphCounts,
                         public StaticAlgorithm {
  private:
    std::vector<int> OnVertexAddId;
    std::vector<int> OnVertexRemoveId;
    std::vector<int> OnArcAddId;
    std::vector<int> OnArcRemoveId;

    SingleKeyMap sTriangle;
    SingleKeyMap sTPath;
    SingleKeyMap sClaw;
    SingleKeyMap sPaw;
    SingleKeyMap sFCycle;
    SingleKeyMap sDiamond;
    SingleKeyMap sFClique;

  public:
    DynamizedESCAPE(Config &config) : defer_updates(config.workers > 1) {}
    ~DynamizedESCAPE() override = default;

    std::string getName() override { return "DynamizedESCAPE"; }

    bool prepare() override;
    void clear();

    void setVertexPartition(VertexPartition *vPart) {};

    std::chrono::duration<double> getCurrentDynTime() override {
        return timeLastComputation;
    }

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

    void write_debug() override {}

    StaticGraphSnapshot snapshot_current_graph() const override;
    unsigned int current_num_vertices() const override {
        return graph->getSize();
    }
    std::vector<StaticGraphUpdate> take_pending_updates() override;
    StaticAlgorithmResult
    compute_snapshot(const StaticGraphSnapshot &snapshot) const override;
    void apply_result(const StaticAlgorithmResult &result) override;

  private:
    std::chrono::duration<double> timeLastComputation;
    bool removeArc{0};
    bool defer_updates{false};
    std::vector<StaticGraphUpdate> pending_updates;

    void run();
    StaticGraphSnapshot capture_snapshot() const;
};

#endif
