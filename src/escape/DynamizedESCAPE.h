#ifndef ESCAPE_SUBGRAPH_COUNTS_H
#define ESCAPE_SUBGRAPH_COUNTS_H

#include "ObserverAlgorithm.h"
#include "UndirectedFourSubgraphCounts.h"
#include "static/StaticSnapshot.h"
#include <algorithm/staticalgorithmwrapper.h>

class DynamizedESCAPE : public UndirectedFourSubgraphCounts {
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
    DynamizedESCAPE(Config &) {}
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

  private:
    std::chrono::duration<double> timeLastComputation;
    bool removeArc{0};
    void run();
    StaticGraphSnapshot capture_snapshot() const;
    StaticAlgorithmResult
    compute_snapshot(const StaticGraphSnapshot &snapshot) const;
    void apply_result(const StaticAlgorithmResult &result);
};

#endif
