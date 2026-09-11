#pragma once

#include "graph.dyn/dynamicdigraph.h"
#include "graph/vertex.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <memory>
#include <string>
#include <vector>

class ObserverAlgorithm {
  protected:
    Algora::IncidenceListGraph *graph;
    std::vector<int> OnVertexAddId;
    std::vector<int> OnVertexRemoveId;
    std::vector<int> OnArcAddId;
    std::vector<int> OnArcRemoveId;

    virtual void cleanup() {}

  public:
    ObserverAlgorithm() : graph(nullptr) {};
    virtual ~ObserverAlgorithm() {
        if (!hasGraph())
            return;
        this->unsetGraph();
    }

    void setGraph(Algora::IncidenceListGraph *InGraph) {
        this->graph = InGraph;
    }
    virtual bool prepare() = 0;

    /**
     * @brief Unset the current Graph and remove all observers associated
     *
     * @return true
     * @return false
     */
    bool unsetGraph() {
        if (!hasGraph())
            return 1;
        for (auto &id : OnVertexAddId)
            graph->removeOnVertexAdd(&id);
        for (auto &id : OnVertexRemoveId)
            graph->removeOnVertexRemove(&id);
        for (auto &id : OnArcAddId)
            graph->removeOnArcAdd(&id);
        for (auto &id : OnArcRemoveId)
            graph->removeOnArcRemove(&id);
        cleanup();
        graph = nullptr;

        return 1;
    }

    bool hasGraph() { return graph != nullptr; }
};
