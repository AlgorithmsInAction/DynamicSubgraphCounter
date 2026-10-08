#ifndef STATIC_SNAPSHOT_H
#define STATIC_SNAPSHOT_H

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

#endif
