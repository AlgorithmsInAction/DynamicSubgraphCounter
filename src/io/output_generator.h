#ifndef OUTPUT_H
#define OUTPUT_H

#include "SubgraphCounts.h"
#include "config.h"
#include "util/streaming_stats.h"

#include <fstream>
#include <iostream>

class OutputGeneration {
  public:
    OutputGeneration(Config &config) : config_ref(config) {
        write_short_stat = config_ref.out_short_stats_file != "";
        write_stat = config_ref.out_stats_file != "";
        write_results = !config.dont_write_counts;
    };

    void open_file_streams(SubgraphCounts *SubCounts) {
        if (write_results) {
            out_result.open(config_ref.out_result_file, std::ios::out);
            out_result << "step," << SubCounts->print_global_count_header()
                       << std::endl;
        }

        if (write_stat) {
            out_stats.open(config_ref.out_stats_file, std::ios::out);
            out_stats << "step," << SubCounts->print_current_stats_header()
                      << std::endl;
        }

        if (write_short_stat) {
            out_short_stats.open(config_ref.out_short_stats_file,
                                 std::ios::app);
            std::string counting = "";
            if (config_ref.counting_stats) {
                if (UndirectedFourSubgraphCounts *fCounts =
                        dynamic_cast<UndirectedFourSubgraphCounts *>(
                            SubCounts)) {
                    std::vector<std::string> pat = {
                        "tCycle",  "tPath",    "claws",   "paws",
                        "fCycles", "diamonds", "fCliques"};

                    std::vector<std::string> metric = {
                        "increased", "decreased", "mean", "min",
                        "max",       "99q",       "1q"};

                    for (const auto &p : pat) {
                        for (const auto &m : metric) {
                            counting += p + "_" + m + ",";
                        }
                    }

                }
            }
            out_short_stats << "graph,steps,algo,partition,"
                            << SubCounts->print_short_stats_header()
                            << "pattern,time,mean_update,min_update,max_update,"
                               "99q_update,1q_update,"
                            << counting
                            << "tCycle_final,diamonds_final,tPath_final,"
                               "fCycles_final,claws_final,fCliques_final,paws_final,"
                            << "memory" << std::endl;
        }
    }
    void close_file_streams() {
        if (write_results)
            out_result.close();

        if (write_stat)
            out_stats.close();

        if (write_short_stat)
            out_short_stats.close();
    }

    void writeStats(int step, SubgraphCounts *SubCounts) {
        if (!write_stat) {
            return;
        }

        out_stats << step << "," << SubCounts->print_current_stats() << "\n";
    }

    void writeResults(int step, SubgraphCounts *SubCounts) {
        if (!write_results) {
            return;
        }
        out_result << step << ","
                   << SubCounts->print_current_output_global_count()
                   << std::endl;
    }

    void writeShortStats(int counter, unsigned long memory,
                         StreamingStats &stats,
                         std::vector<StreamingStats> &counting_stats,
                         SubgraphCounts *SubCounts) {
        if (!write_short_stat) {
            return;
        }
        auto graph = config_ref.input_graph_file;

        size_t posEnd = graph.rfind('.');   // find the last dot
        size_t posStart = graph.rfind('/'); // find the last dot
        if (posEnd != std::string::npos) {
            graph = graph.substr(0, posEnd);
        }
        if (posStart != std::string::npos) {
            graph = graph.substr(posStart + 1);
        }

        std::string pattern;

        if (config_ref.count_all) {
            pattern = "all";
        } else {
            std::vector<std::pair<std::string, bool>> flags = {
                {"tCycles", config_ref.count_tCycles},
                {"tPaths", config_ref.count_tPaths},
                {"claws", config_ref.count_claws},
                {"paws", config_ref.count_paws},
                {"fCycles", config_ref.count_fCycles},
                {"diamonds", config_ref.count_diamonds},
                {"fCliques", config_ref.count_fCliques}};

            // Concatenate names of true flags
            for (const auto &[name, flag] : flags) {
                if (flag)
                    pattern += name;
            }
        }

        out_short_stats << graph << "," << counter << ","
                        << algoNames[config_ref.algo] << ","
                        << partNames[config_ref.part] << ","
                        << SubCounts->print_short_stats() << pattern << ","
                        << stats.sum() << "," << stats.mean() << ","
                        << stats.min() << "," << stats.max() << ","
                        << stats.quantile(0.99) << "," << stats.quantile(0.01)
                        << ",";

        if (config_ref.counting_stats) {
            for (auto stat : counting_stats) {
                out_short_stats
                    << stat.increased() << "," << stat.decreased() << ","
                    << stat.mean() << "," << stat.min() << "," << stat.max()
                    << "," << stat.quantile(0.99) << "," << stat.quantile(0.01)
                    << ",";
            }
        }

        out_short_stats << SubCounts->print_current_output_global_count()
                        << ",";
        out_short_stats << memory << "\n";
    }

  private:
    Config &config_ref;

    bool write_short_stat;
    bool write_stat;
    bool write_results;

    std::ofstream out_result;
    std::ofstream out_stats;
    std::ofstream out_short_stats;
};

#endif
