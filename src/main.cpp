#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include "SubgraphCounts.h"
#include "algorithm.basic.traversal/depthfirstsearch.h"
#include "config.h"
#include "definitions.h"
#include "get_algorithm.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph.incidencelist/incidencelistvertex.h"
#include "graph/arc.h"
#include "graph/digraph.h"
#include "graph/graph_functional.h"
#include "graph/vertex.h"
#include "io/konectnetworkreader.h"
#include "io/output_generator.h"
#include "io/parse_paramters.h"
#include "property/fastpropertymap.h"
#include "util/graph_stats.h"
#include "util/memory.h"
#include "util/streaming_stats.h"

using namespace Algora;

double readDynamicGraph(DynamicDiGraph &dyGraph, const std::string &filename,
                        const int lifetime_int);
bool printInfos(const DynamicDiGraph &dyGraph, const double &time);
int main(int argc, char *argv[]) {

    auto baseline = getPeakRSS();

    // Command line options and corresponding variables

    Config config;
    parse_algorithm(argc, argv, config);

    OutputGeneration out_gen(config);
    DynamicDiGraph dyGraph;
    double time =
        readDynamicGraph(dyGraph, config.input_graph_file, config.lifetime);
    if (time == -1)
        return 1;

    SubgraphCounts *SubCounts = get_algorithm(config);
    VertexPartition *vertexPartition = get_partition(config);

    SubCounts->setGraph(dyGraph.getDiGraph());
    SubCounts->setVertexPartition(vertexPartition);
    SubCounts->prepare();

    out_gen.open_file_streams(SubCounts);

    int counter = 0;

    StreamingStats total_stats;
    GraphStats graph_stats;

    std::vector<StreamingStats> counting_stats(7, 0);

    auto write_step = [&](int step) {
        auto currentDynTime = SubCounts->getCurrentDynTime();
        total_stats.add(currentDynTime.count());

        out_gen.writeStats(step, SubCounts);
        out_gen.writeResults(step, SubCounts);

        if (config.print_debug)
            SubCounts->write_debug();

        if (config.counting_stats) {
            if (UndirectedFourSubgraphCounts *fCounts =
                    dynamic_cast<UndirectedFourSubgraphCounts *>(SubCounts)) {
                counting_stats[0].add(fCounts->getNrtCycle());
                counting_stats[1].add(fCounts->getNrTPaths());
                counting_stats[2].add(fCounts->getNrClaws());
                counting_stats[3].add(fCounts->getNrPaws());
                counting_stats[4].add(fCounts->getNrfCycles());
                counting_stats[5].add(fCounts->getNrDiamonds());
                counting_stats[6].add(fCounts->getNrFCliques());
            }
        }
    };

    if (config.print_graph_stats) {
        graph_stats.setGraph(dyGraph.getDiGraph());
        graph_stats.setConfig(config);
        graph_stats.setSubgraphCounts(SubCounts, vertexPartition);
        graph_stats.prepare();
    }

    while (dyGraph.applyNextOperation()) {
        ++counter;
        if (config.print_debug)
            std::cout << "Change " << counter << std::endl;
        write_step(counter);
        if (config.timeout_in_s && config.timeout_in_s < total_stats.sum())
            break;
    }
    if (config.print_graph_stats) {
        graph_stats.printStats();
    }
    auto memory = getPeakRSS() - baseline;

    out_gen.writeShortStats(counter, memory, total_stats, counting_stats,
                            SubCounts);

    SubCounts->unsetGraph();

    delete SubCounts;
    delete vertexPartition;
}
/**
 * @brief Reads a dynamic graph from a file to the given dyGraph object.
 * Required Format: Tail-ID Head-ID Delta-ID +/-1
 *
 * @param dyGraph Object, the graph is written into
 * @param filename Name of the input file
 * @param lifetime_int Lifetime of the edges. Ignored of set to 0
 * @return double
 */
double readDynamicGraph(DynamicDiGraph &dyGraph, const std::string &filename,
                        const int lifetime_int) {
    KonectNetworkReader reader(true);
    reader.setStrict(true);
    reader.setAllUndirected(true);
    std::ifstream input(filename, std::ifstream::in);
    if (!input) {
        std::cerr << "Could not open input file " << filename << std::endl;
        return -1;
    }
    std::chrono::duration<double> elapsed_time;
    if (lifetime_int > 0)
        reader.setArcLifetime(lifetime_int);
    reader.setInputStream(&input);
    if (reader.isGraphAvailable()) {
        auto start = std::chrono::high_resolution_clock::now();
        bool done = reader.provideDynamicDiGraph(&dyGraph);
        auto end = std::chrono::high_resolution_clock::now();
        elapsed_time = end - start;
        if (done) {
            auto errors = reader.getErrors();
            if (!errors.empty()) {
                std::cerr << "Warnings: " << errors;
            }
        } else {
            std::cerr << "Errors occurred while reading graph." << std::endl;
            auto errors = reader.getErrors();
            if (!errors.empty()) {
                std::cerr << "Errors: " << errors;
            }
        }
    } else {
        std::cerr << "No graph available. Does the file store a dynamic graph?"
                  << std::endl;
        return -1;
    }
    // std::cout << "Graph read\n";
    return elapsed_time.count();
}
/**
 * @brief Writes some statistics about the read graph to the console
 *
 * @param dyGraph Graph to use for the statistics
 * @param time Time required to read the graph
 * @return true
 * @return false
 */
bool printInfos(const DynamicDiGraph &dyGraph, const double &time) {
    std::cout << "\n----------Graph Informations----------\n" << std::endl;
    std::cout << "Read-Time: " << time << " seconds\n";
    std::cout << "Max Timestep: " << dyGraph.getMaxTime() << std::endl;
    std::cout << dyGraph.countArcAdditions(0, dyGraph.getMaxTime())
              << " arcs and "
              << dyGraph.countVertexAdditions(0, dyGraph.getMaxTime())
              << " nodes added " << std::endl;
    std::cout << dyGraph.countArcRemovals(0, dyGraph.getMaxTime())
              << " arcs and "
              << dyGraph.countVertexRemovals(0, dyGraph.getMaxTime())
              << " nodes removed " << std::endl;
    std::cout << "Final Nr of Nodes: " << dyGraph.getConstructedGraphSize()
              << std::endl;
    std::cout << "Final Nr of Arcs: " << dyGraph.getConstructedArcSize()
              << std::endl;

    return true;
}
