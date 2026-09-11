#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

enum Algo { HHH, HHH_VANILLA, EGST, OB, ESCAPE };

enum Partition { EPS_TAB, HINDEX, MOCK, NONE };

const std::vector<std::string> algoNames = {"hhh", "v_hhh", "egst", "oba",
                                            "escape"};
const std::vector<std::string> partNames = {"epstab", "hindex", "mock", "none"};

struct Config {
  public:
    Algo algo;
    Partition part;

    // Pattern Settings
    bool count_tCycles{false};
    bool count_tPaths{false};
    bool count_claws{false};
    bool count_paws{false};
    bool count_fCycles{false};
    bool count_diamonds{false};
    bool count_fCliques{false};
    bool count_all{false};

    // Mode
    bool g_counts{true};  // Calculate the total counts or
    bool s_counts{false}; // Calculate the s-counts (for every vertex)

    std::vector<int> v_list; // vertices_list_for_s_count

    // Computation settings
    double epsilon;
    int lifetime{0}; // Set fixed lifetime for edges
    double timeout_in_s{0};
    unsigned int workers{1}; // Parallel workers for static algorithms
    unsigned int worker_block_size{1024};

    double recomputeFactor{2}; // Factor of how often to recompute
    bool soft_recompute{0};
    bool lazy_recompute{0};
    bool delayed_recompute{0};
    int round_robin_size{0};
    double round_robin_percent{0};
    bool epstab_notify_before_change{0};
    bool maintain_part_stats{0};

    double hind_gradual_factor{2}; // Factor of how often to recompute

    bool use_all_possible_structures{false};
    bool use_no_aux_for_t{false};

    bool egst_direct{0};
    bool high_anchors_only{0};

    // I/O
    std::string input_graph_file;

    std::string out_result_file = "out.csv";
    std::string out_stats_file;
    std::string out_short_stats_file;

    bool print_graph_stats{false};
    bool counting_stats{false};
    bool print_debug{false};
    bool dont_write_counts{false};

    // STATS options
    bool storeNumRecomputations{false};
};

#endif
