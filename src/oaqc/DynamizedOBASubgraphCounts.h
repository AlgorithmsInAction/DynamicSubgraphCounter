#ifndef OBA_SUBGRAPH_COUNTS_H
#define OBA_SUBGRAPH_COUNTS_H

#include "ObserverAlgorithm.h"
#include "UndirectedFourSubgraphCounts.h"
#include "h_counts/HStructureCounts.h"
#include "static/StaticSnapshot.h"
#include <algorithm/staticalgorithmwrapper.h>

class DynamizedOBASubgraphCounts : public UndirectedFourSubgraphCounts {
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
    DynamizedOBASubgraphCounts(Config &) {}
    ~DynamizedOBASubgraphCounts() override = default;

    std::string getName() override { return "HSubgraphCounts"; }

    bool prepare() override;
    void clear();

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

    void setVertexPartition(VertexPartition *vPart) {};

    std::chrono::duration<double> getCurrentDynTime() override {
        return timeLastComputation;
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
