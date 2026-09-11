#ifndef STATIC_ALGORITHM_H
#define STATIC_ALGORITHM_H

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

struct StaticGraphSnapshot {
    unsigned int num_vertices{0};
    // The first half contains tails and the second half the corresponding
    // heads, which is the input layout expected by OB.
    std::vector<int> edges;

    unsigned int num_edges() const {
        return static_cast<unsigned int>(edges.size() / 2);
    }
};

struct StaticAlgorithmResult {
    // triangle, diamond, three-path, four-cycle, claw, four-clique, paw
    std::array<std::uint64_t, 7> counts{};
    std::chrono::duration<double> elapsed{0};
};

struct StaticGraphUpdate {
    int tail;
    int head;
    bool insertion;
};

struct StaticUpdateStep {
    unsigned int num_vertices{0};
    std::vector<StaticGraphUpdate> updates;
};

struct StaticUpdateBlock {
    int first_step{0};
    StaticGraphSnapshot checkpoint;
    std::vector<StaticUpdateStep> steps;
};

/**
 * Common snapshot interface for the two static algorithms.  Capturing a
 * snapshot remains in the graph callback (where removal ordering matters),
 * while the expensive, graph-independent computation can run on a worker.
 */
class StaticAlgorithm {
  public:
    virtual ~StaticAlgorithm() = default;

    virtual StaticGraphSnapshot snapshot_current_graph() const = 0;
    virtual unsigned int current_num_vertices() const = 0;
    virtual std::vector<StaticGraphUpdate> take_pending_updates() = 0;
    virtual StaticAlgorithmResult
    compute_snapshot(const StaticGraphSnapshot &snapshot) const = 0;
    virtual void apply_result(const StaticAlgorithmResult &result) = 0;
};

#endif
