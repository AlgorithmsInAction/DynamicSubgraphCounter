#ifndef PARSE_PARAMETER_H
#define PARSE_PARAMETER_H

#include "config.h"
#include "io/CLI11.hpp"

bool parse_algorithm(int argc, char *argv[], Config &config) {
    CLI::App app{"UserInterface"};

    app.add_option("-i,--input", config.input_graph_file, "Dynamic Graph Input")
        ->required();

    std::string algo;
    app.add_option("-a,--algo", algo, "Algorithm to count subgraphs")
        ->required();

    std::string part;
    app.add_option("-p,--partition", part, "Vertex partition");

    app.add_option("-e,--epsilon", config.epsilon, "Value for Epsilon");

    app.add_option("-o,--result_file", config.out_result_file,
                   "Output file for counting results")
        ->capture_default_str();
    app.add_option("-f,--stats_file", config.out_stats_file,
                   "Output file for computation statistics per step");
    app.add_option("--ssf,--short_stats_file", config.out_short_stats_file,
                   "Output file for summed computation statistics");

    std::string vertices;
    app.add_option("-v,--vertices", vertices,
                   "List of vertices to calculate for separated by ,");
    app.add_flag("-s,--s_counts", config.s_counts,
                 "Calculate the s-counts (for every vertex)");

    app.add_option("--lifetime", config.lifetime,
                   "Sets fixed lifetime for edges");
    app.add_option("-r,--rebalance_factor", config.recomputeFactor,
                   "Factor of how often to recompute, Must be > 1");

    std::string recomp_mode;
    app.add_option("-m,--rebalance_mode", recomp_mode,
                   "Use recompute mode for epstab: options: soft, lazy, late");
    app.add_option(
        "--rr_s", config.round_robin_size,
        "Set size of how many vertices to check in round robin style");
    app.add_option(
        "--rr_p", config.round_robin_percent,
        "Set percentage of how many vertices to check in round robin style");
    app.add_flag(
        "--early-notify", config.epstab_notify_before_change,
        "Use early-notification of inserts before partchanges for epstab");

    app.add_option("--gradual_factor", config.hind_gradual_factor,
                   "Factor x of deg>=x*h_index are consideres high degree , "
                   "Should be > 1");

    app.add_flag("--partition-stats", config.maintain_part_stats,
                 "Print the partition change stats in the short stats file");
    app.add_flag(
        "--extraAux", config.use_all_possible_structures,
        "Only maintain and use all possible aux structures for given patterns");
    app.add_flag("--no_aux_for_t", config.use_no_aux_for_t,
                 "Do not use auxiliary structures for triangles");

    app.add_flag("--direct", config.egst_direct,
                 "Use soft vertex change in egst");
    app.add_flag("--highAnchorsOnly", config.high_anchors_only,
                 "Use soft vertex change in egst");

    std::string stats;
    app.add_option("--stats", stats,
                   "List parameters to include stats for. (Options: r)");

    app.add_flag("--graph_stats", config.print_graph_stats,
                 "Display Graph Infos");
    app.add_flag("--count_stats", config.counting_stats,
                 "Generate counting stats");
    app.add_flag("--debug", config.print_debug,
                 "Display all infos after every step");
    app.add_flag("--dont_write_counts", config.dont_write_counts,
                 "Don't write the results to a file");
    app.add_flag("--dont_write_stats", config.dont_write_counts,
                 "Don't write the stats to a file");

    app.add_option("--timeout", config.timeout_in_s,
                   "Time in hours after what to stop");
    app.add_option("-w,--worker", config.workers,
                   "Workers for parallel static updates (OB or ESCAPE)")
        ->check(CLI::Range(1u, 1024u));
    app.add_option("--worker-block-size", config.worker_block_size,
                   "Updates per precollected static worker block")
        ->check(CLI::Range(1u, 1000000u));

    app.add_flag("--triangle", config.count_tCycles, "Calculate triangles");
    app.add_flag("--tPath", config.count_tPaths, "Calculate three paths");
    app.add_flag("--claw", config.count_claws, "Calculate claws");
    app.add_flag("--paw", config.count_paws, "Calculate paws");
    app.add_flag("--fCycle", config.count_fCycles, "Calculate four cycles");
    app.add_flag("--diamond", config.count_diamonds, "Calculate diamonds");
    app.add_flag("--fClique", config.count_fCliques, "Calculate four cliques");
    app.add_flag("--all", config.count_all, "Calculate all subgraphs");

    CLI11_PARSE(app, argc, argv);

    if (config.use_all_possible_structures && config.use_no_aux_for_t) {
        std::cerr << "--extraAux and --no_aux_for_t cannot be used together\n";
        exit(1);
    }

    if (config.timeout_in_s) {
        config.timeout_in_s = config.timeout_in_s * 3600;
    }

    if (config.count_all) {
        config.count_tCycles = true;
        config.count_tPaths = true;
        config.count_claws = true;
        config.count_paws = true;
        config.count_fCycles = true;
        config.count_diamonds = true;
        config.count_fCliques = true;
    }

    std::stringstream vertexStream(vertices);

    while (!vertexStream.eof()) {
        std::string substring;
        getline(vertexStream, substring, ',');
        if (substring == "")
            continue;
        config.v_list.push_back(std::stoi(substring));
    }

    std::stringstream stats_stream(stats);
    while (!stats_stream.eof()) {
        std::string substring;
        getline(stats_stream, substring, ',');
        if (substring == "")
            continue;
        if (substring == "r") {
            config.storeNumRecomputations = true;
            continue;
        }
    }

    if (algo == "hhh") {
        config.algo = Algo::HHH_VANILLA;
        if (config.high_anchors_only) {
            config.algo = Algo::HHH;
        }
    } else if (algo == "egst") {
        config.algo = Algo::EGST;
    } else if (algo == "ob") {
        config.algo = Algo::OB;
    } else if (algo == "escape") {
        config.algo = Algo::ESCAPE;
    } else {
        std::cerr << "Unknown algorithm: \'" << algo << "\'\n";
        exit(0);
    }

    if (config.workers > 1 && config.algo != Algo::OB &&
        config.algo != Algo::ESCAPE) {
        std::cerr << "--worker is only supported for static algorithms "
                     "'ob' and 'escape'\n";
        exit(1);
    }

    if (part == "epstab") {
        config.part = Partition::EPS_TAB;
    } else if (part == "hindex") {
        config.part = Partition::HINDEX;
    } else if (part == "mock") {
        config.part = Partition::MOCK;
    } else if (config.algo == Algo::OB || config.algo == Algo::ESCAPE) {
        config.part = Partition::NONE;
    } else {
        std::cerr << "A valid partition is required. Unknown partition:  \'"
                  << part << "\'\n";
        exit(0);
    }

    if (recomp_mode == "soft") {
        config.soft_recompute = true;
    } else if (recomp_mode == "lazy") {
        config.lazy_recompute = true;
    } else if (recomp_mode == "late") {
        config.delayed_recompute = true;
    }

    return true;
}
#endif
