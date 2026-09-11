#pragma once

#include "ObserverAlgorithm.h"
#include "definitions.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "graph/digraph.h"
#include "graph/graph_functional.h"
#include "graph/graphartifact.h"
#include "graph/vertex.h"
#include "observable.h"
#include "property/fastpropertymap.h"
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "util/streaming_stats.h"

#define DEBUG_ENABLED 0
#define PARTITION_TIMER 1

#if DEBUG_ENABLED
#define DEBUG_PRINT(fmt, ...)                                                  \
    do {                                                                       \
        std::printf("[DEBUG] " fmt "\n", ##__VA_ARGS__);                       \
        fflush(stdout);                                                        \
    } while (0)
#else
#define DEBUG_PRINT(fmt, ...)                                                  \
    do {                                                                       \
    } while (0)
#endif

class VertexPartition : public ObserverAlgorithm {
    using HighDegMap = boost::unordered_flat_map<int, Algora::Vertex *>;
    using AdjSet =
        boost::unordered_flat_map<std::pair<int, int>, std::monostate>;

  protected:
    Algora::Arc *removedArc = nullptr;
    COUNTER_TYPE nr_edges;
    COUNTER_TYPE M;

    double theta;
    bool maintain_part_stats{false};
    COUNTER_TYPE num_part_changes_per_step{0};
    StreamingStats highSizeStats;
    StreamingStats changesPerUpdateStats;
#ifdef PARTITION_TIMER
    std::chrono::duration<double> timePartition{0};
#endif

    HighDegMap HighDeg;
    // boost::unordered_map<int, const Algora::Vertex *> HighDeg;
    // Arcs of the given vertex with smaller ID lead to a high-degree vertex
    // TODO: NOOO
    boost::unordered_map<const Algora::Vertex *, int> arcBorder;
    AdjSet adjacencyMap;

    Algora::Observable<ILV *> observableVertexUp;
    Algora::Observable<ILV *> observableVertexDown;

    Algora::Observable<ILV *> observableBeforeVertexUp;
    Algora::Observable<ILV *> observableBeforeVertexDown;

    Algora::Observable<Algora::Arc *> observableArcAdd;
    Algora::Observable<Algora::Arc *> observableArcRemove;

    std::chrono::duration<double> currentDynTime{0};

  public:
    COUNTER_TYPE edge_insertions{0};
    COUNTER_TYPE edge_removals{0};
    bool fixed{false};

  public:
    VertexPartition() = default;
    virtual ~VertexPartition() = default;

    virtual bool prepare() = 0;
    virtual void cleanup() = 0;
    virtual void print_table() = 0;

    virtual std::string print_current_stats_header() = 0;
    virtual std::string print_current_stats() = 0;

    virtual std::string print_short_stats() = 0;
    virtual std::string print_short_stats_header() = 0;

    virtual void EpsTabOnArcRemove(Algora::Arc *a) = 0;
    virtual void EpsTabOnArcAdd(Algora::Arc *a) = 0;
    virtual void EpsTabOnVertexRemove(ILV *v) = 0;

    std::chrono::duration<double> getCurrentDynTime() { return currentDynTime; }

    const Algora::Arc *getRemovedArc() { return removedArc; }

    std::pair<int, int> getKey(int head, int tail) {
        if (head < tail) {
            return std::pair<int, int>({head, tail});
        } else {
            return std::pair<int, int>({tail, head});
        }
    }
    std::pair<int, int> getKey(Algora::Arc *a) {
        return getKey(a->getHead()->getId(), a->getTail()->getId());
    }
    /**
     * @brief Fixes the Epsilon Table.
     * Any edge insertions or removals will not be notices and also
     * not be forwarded via any observers.
     *
     */
    void fixTable() { fixed = true; }

    /**
     * @brief Unfixes the Epsilon Table.
     *
     */
    void unfixTable() { fixed = false; }
    /**
     * @brief Set the On Recompute object, which will be called
     * in case of a recomputation after the Epsilon Partition was
     * recalculated.
     *
     * @param on_recompute_func
     */
    virtual void setOnRecompute(
        std::function<
            void(bool arcAdd,
                 boost::unordered_map<const Algora::Vertex *, int> &arcBorder)>
            on_recompute_func) = 0;
    virtual void removeOnRecompute() = 0;

    bool is_high_deg(const Algora::Vertex::id_type id) {

        if (!hasGraph())
            throw std::runtime_error("Need to set Graph first");
        return (HighDeg.find(id) != HighDeg.end());
    }

    bool is_high_deg(const Algora::Vertex *v) {
        return is_high_deg(v->getId());
    }
    bool is_low_deg(const Algora::Vertex::id_type id) {
        return !is_high_deg(id);
    }
    bool is_low_deg(const Algora::Vertex *v) { return !is_high_deg(v); }

    // Returns the iterator to the start/end of the vector that contains all
    // high degree vertices.
    HighDegMap::const_iterator highDegVBegin() const { return HighDeg.begin(); }
    HighDegMap::const_iterator highDegVEnd() const { return HighDeg.end(); }

    /**
     * @brief Determines if an edge exists between the two given vertices
     *
     * @param v1 vertex 1
     * @param v2 vertex 2
     * @return true
     * @return false
     */
    inline bool arcExists(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto key = getKey(v1->getId(), v2->getId());
        return !(adjacencyMap.find(key) == adjacencyMap.end());
    }

    inline void adjacencyInsert(Algora::Arc *a) {
        auto key = getKey(a);
        adjacencyMap.emplace(key, std::monostate{});
    }

    inline void adjacencyDelete(Algora::Arc *a) {
        auto key = getKey(a);
        adjacencyMap.erase(key);
    }

    inline void adjacencyDeleteAll() { adjacencyMap.clear(); }

    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getHighDegBegin(ILV *v) {
        return v->getEdges().begin();
    }
    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getHighDegEnd(ILV *v) {
        return v->getEdges().begin() + arcBorder[v];
    }
    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getLowDegBegin(ILV *v) {
        return v->getEdges().begin() + arcBorder[v];
    }
    inline __gnu_cxx::__normal_iterator<Algora::Arc *const *,
                                        std::vector<Algora::Arc *>>
    getLowDegEnd(ILV *v) {
        return v->getEdges().end();
    }

    inline int getHighDegree(ILV *v) { return arcBorder[v]; };

    inline int getLowDegree(ILV *v) {
        return v->getUndirectedDegree() - arcBorder[v];
    };

    inline int getLowDegreeWithoutRemoved(ILV *v) {
        return v->getUndirectedDegree() - arcBorder[v] -
               (removedArc != nullptr &&
                (removedArc->getFirst() == v || removedArc->getSecond() == v));
    }

    inline int getDegreeWithoutRemoved(ILV *v) {
        return v->getUndirectedDegree() -
               (removedArc != nullptr &&
                (removedArc->getFirst() == v || removedArc->getSecond() == v));
    }

    template <typename Func>
    void forLowerHighDegree(ILV *v1, ILV *v2, Func operation) {
        int d1 = arcBorder[v1];
        int d2 = arcBorder[v2];

        if (d1 < d2) {
            operation(v1, v2);
        } else {
            operation(v2, v1);
        }
    }

    template <typename Func>
    void forLowLowerDegree(ILV *v1, ILV *v2, Func operation) {
        int d1 = graph->getUndirectedDegree(v1);
        int d2 = graph->getUndirectedDegree(v2);

        if (is_low_deg(v1) && (is_high_deg(v2) || d1 <= d2)) {
            operation(v1, v2);
        } else {
            operation(v2, v1);
        }
    }

    int get_num_high_deg() { return HighDeg.size(); }
    COUNTER_TYPE get_m() { return nr_edges; }

    virtual int getTheoreticalMaxLowDegree() = 0;
    virtual int getEpsilon() = 0;
    virtual int getNumRecomputations() = 0;

    virtual std::chrono::duration<double> getTotalRecomputationTime() {
        return std::chrono::duration<double>::zero();
    }

    /**
     * @brief Adds the given function to the observer, that activates before
     * a vertex changes form low to high degree
     *
     * @param id ID of the function
     * @param vvFun function to call
     */
    void onBeforeVertexToHigh(void *id, const Algora::VertexMapping &vvFun) {
        this->observableBeforeVertexUp.addObserver(id, vvFun);
    }

    /**
     * @brief Adds the given function to the observer, that activates before
     * a vertex changes form high to low degree
     *
     * @param id ID of the function
     * @param vvFun function to call
     */
    void onBeforeVertexToLow(void *id, const Algora::VertexMapping &vvFun) {
        this->observableBeforeVertexDown.addObserver(id, vvFun);
    }

    /**
     * @brief Removes the function with the given ID from the
     * observer that activates if a vertex changes from
     * low to high degree
     *
     * @param id ID of the function
     */
    void removeOnBeforeVertexToHigh(void *id) {
        this->observableBeforeVertexUp.removeObserver(id);
    }

    /**
     * @brief Removes the function with the given ID from the
     * observer that activates if a vertex changes from
     * high to low degree
     *
     * @param id ID of the function
     */
    void removeOnBeforeVertexToLow(void *id) {
        this->observableBeforeVertexDown.removeObserver(id);
    }
    /**
     * @brief Adds the given function to the observer, that activates if
     * a vertex changes form low to high degree
     *
     * @param id ID of the function
     * @param vvFun function to call
     */
    void onVertexToHigh(void *id, const Algora::VertexMapping &vvFun) {
        this->observableVertexUp.addObserver(id, vvFun);
    }

    /**
     * @brief Adds the given function to the observer, that activates if
     * a vertex changes form high to low degree
     *
     * @param id ID of the function
     * @param vvFun function to call
     */
    void onVertexToLow(void *id, const Algora::VertexMapping &vvFun) {
        this->observableVertexDown.addObserver(id, vvFun);
    }

    /**
     * @brief Adds the given function to the observer, that activates if
     * an edge is inserted. Doesn't call the function if the
     * Epsilon Table is fixed
     *
     * @param id ID of the function
     * @param vvFun function to call
     */
    void onArcAdd(void *id, const Algora::ArcMapping &vvFun) {
        this->observableArcAdd.addObserver(id, vvFun);
    }

    /**
     * @brief Adds the given function to the observer, that activates if
     * an edge is removed. Doesn't call the function if the
     * Epsilon Table is fixed
     *
     * @param id ID of the function
     * @param vvFun function to call
     */
    void onArcRemove(void *id, const Algora::ArcMapping &vvFun) {
        this->observableArcRemove.addObserver(id, vvFun);
    }

    /**
     * @brief Removes the function with the given ID from the
     * observer that activates if a vertex changes from
     * low to high degree
     *
     * @param id ID of the function
     */
    void removeOnVertexToHigh(void *id) {
        this->observableVertexUp.removeObserver(id);
    }

    /**
     * @brief Removes the function with the given ID from the
     * observer that activates if a vertex changes from
     * high to low degree
     *
     * @param id ID of the function
     */
    void removeOnVertexToLow(void *id) {
        this->observableVertexDown.removeObserver(id);
    }

    /**
     * @brief Removes the function with the given ID from the
     * observer that activates if an edge is inserted
     *
     * @param id ID of the function
     */
    void removeOnArcAdd(void *id) { this->observableArcAdd.removeObserver(id); }

    /**
     * @brief Removes the function with the given ID from the
     * observer that activates if an edge is removed
     *
     * @param id ID of the function
     */
    void removeOnArcRemove(void *id) {
        this->observableArcRemove.removeObserver(id);
    }

    void updateArcBorderOnInsert(Algora::Arc *a) {
#ifdef PARTITION_TIMER
        auto start = std::chrono::high_resolution_clock::now();
#endif
        ILV *head = CAST_ILV(a->getHead());
        ILV *tail = CAST_ILV(a->getTail());

        int headIndex = head->undirectedIndexOf(a);
        int tailIndex = tail->undirectedIndexOf(a);
        bool headHigh = is_high_deg(head);
        bool tailHigh = is_high_deg(tail);

        if (tailHigh && headIndex >= arcBorder[head])
            head->swapEdges(headIndex, arcBorder[head]++);
        else if (tailHigh && headIndex < arcBorder[head])
            arcBorder[head]++;
        else if (!tailHigh && headIndex < arcBorder[head])
            head->swapEdges(headIndex, arcBorder[head]);

        if (headHigh && tailIndex >= arcBorder[tail])
            tail->swapEdges(tailIndex, arcBorder[tail]++);
        else if (headHigh && tailIndex < arcBorder[tail])
            arcBorder[tail]++;
        else if (!headHigh && tailIndex < arcBorder[tail])
            tail->swapEdges(tailIndex, arcBorder[tail]);

        assert(arcBorder[head] <= head->getUndirectedDegree());
        assert(arcBorder[tail] <= tail->getUndirectedDegree());
#ifdef PARTITION_TIMER
        auto end = std::chrono::high_resolution_clock::now();
        timePartition += end - start;
#endif
    }

    void insertVertexIntoHigh(ILV *v, ILV *dont_update = nullptr) {

        if (is_high_deg(v)) {
            return;
        }

        this->observableBeforeVertexUp.notifyObservers(v);
#ifdef PARTITION_TIMER
        auto start = std::chrono::high_resolution_clock::now();
#endif

        HighDeg.insert({v->getId(), v});
        ++num_part_changes_per_step;
        for (auto const &a : v->getEdges()) {
            if (a == removedArc)
                continue;

            auto x = CAST_ILV(a->getOther(v));
            if (x == dont_update)
                continue;
            x->swapEdges(x->undirectedIndexOf(a), arcBorder[x]++);

            assert(arcBorder[x] <= x->getUndirectedDegree());
        }

#ifdef PARTITION_TIMER
        auto end = std::chrono::high_resolution_clock::now();
        timePartition += end - start;
#endif
        this->observableVertexUp.notifyObservers(v);
    }

    void updateArcBorderOnRemove(Algora::Arc *a) {
#ifdef PARTITION_TIMER
        auto start = std::chrono::high_resolution_clock::now();
#endif
        ILV *head = CAST_ILV(a->getHead());
        ILV *tail = CAST_ILV(a->getTail());
        int headIndex = head->undirectedIndexOf(a);
        int tailIndex = tail->undirectedIndexOf(a);
        bool headHigh = is_high_deg(head);
        bool tailHigh = is_high_deg(tail);

        assert(arcBorder[head] <= head->getUndirectedDegree());
        assert(arcBorder[tail] <= tail->getUndirectedDegree());

        if (tailHigh)
            head->swapEdges(headIndex, --arcBorder[head]);
        if (headHigh)
            tail->swapEdges(tailIndex, --arcBorder[tail]);

        assert(arcBorder[head] >= 0);
        assert(arcBorder[tail] >= 0);
#ifdef PARTITION_TIMER
        auto end = std::chrono::high_resolution_clock::now();
        timePartition += end - start;
#endif
    }

    bool removeVertexFromHigh(ILV *v, ILV *dont_update = nullptr) {
        if (!is_high_deg(v)) {
            return false;
        }

        this->observableBeforeVertexDown.notifyObservers(v);
#ifdef PARTITION_TIMER
        auto start = std::chrono::high_resolution_clock::now();
#endif

        HighDeg.erase(v->getId());
        ++num_part_changes_per_step;
        for (auto const &a : v->getEdges()) {
            if (a == removedArc)
                continue;
            auto x = CAST_ILV(a->getOther(v));
            if (x == dont_update)
                continue;
            x->swapEdges(x->undirectedIndexOf(a), --arcBorder[x]);

            assert(arcBorder[x] >= 0);
        }
#ifdef PARTITION_TIMER
        auto end = std::chrono::high_resolution_clock::now();
        timePartition += end - start;
#endif

        this->observableVertexDown.notifyObservers(v);

        return true;
    }

    void updateStatsStart() { num_part_changes_per_step = 0; }

    void updateStats() {
        if (!maintain_part_stats) {
            return;
        }

        highSizeStats.add(HighDeg.size());
        changesPerUpdateStats.add(num_part_changes_per_step);
    }

    std::string partChangeHeader() {
        if (!maintain_part_stats) {
            return "";
        }
        return "insertions,removals,num_part_changes,changes_mean,changes_max,"
               "changes_99q,"
               "changes_1q,H_size_mean,H_size_max,H_size_99q,H_size_1q,"
#ifdef PARTITION_TIMER
               "partition_time,"
#endif
            ;
    }

    std::string partChanges() {
        if (!maintain_part_stats) {
            return "";
        }
        return std::to_string(edge_insertions) + "," +
               std::to_string(edge_removals) + "," +
               std::to_string(changesPerUpdateStats.sum()) + "," +
               std::to_string(changesPerUpdateStats.mean()) + "," +
               std::to_string(changesPerUpdateStats.max()) + "," +
               std::to_string(changesPerUpdateStats.quantile(0.99)) + "," +
               std::to_string(changesPerUpdateStats.quantile(0.01)) + "," +
               std::to_string(highSizeStats.mean()) + "," +
               std::to_string(highSizeStats.max()) + "," +
               std::to_string(highSizeStats.quantile(0.99)) + "," +
               std::to_string(highSizeStats.quantile(0.01)) + ","
#ifdef PARTITION_TIMER
               + std::to_string(timePartition.count()) + ","

#endif
            ;
    }
};
