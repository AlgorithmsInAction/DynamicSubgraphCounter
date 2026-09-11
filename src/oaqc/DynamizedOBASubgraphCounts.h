#ifndef OBA_SUBGRAPH_COUNTS_H
#define OBA_SUBGRAPH_COUNTS_H

#include "ObserverAlgorithm.h"
#include "UndirectedFourSubgraphCounts.h"
#include "h_counts/HStructureCounts.h"
#include "static/StaticAlgorithm.h"
#include <algorithm/staticalgorithmwrapper.h>

class DynamizedOBASubgraphCounts : public UndirectedFourSubgraphCounts,
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
    DynamizedOBASubgraphCounts(Config &config)
        : defer_updates(config.workers > 1) {}
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
