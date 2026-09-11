#include <bits/stdc++.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct pair_hash {
    size_t operator()(const std::pair<int, int> &p) const {
        return ((uint64_t)p.first << 32) ^ (uint64_t)p.second;
    }
};

struct Event {
    int u, v;
    int action;
    int timestamp;
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = input_file + ".e";

    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file.\n";
        return 1;
    }

    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file.\n";
        return 1;
    }

    // Read all events
    std::vector<Event> events;
    std::string line;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        Event ev;
        ss >> ev.u >> ev.v >> ev.action >> ev.timestamp;
        events.push_back(ev);
    }
    infile.close();

    // Sort by timestamp
    std::stable_sort(events.begin(), events.end(),
                     [](const Event &a, const Event &b) {
                         return a.timestamp < b.timestamp;
                     });

    std::unordered_map<std::pair<int, int>, int, pair_hash> inserted;

    // Process events in time order
    for (const auto &ev : events) {
        int u = ev.u;
        int v = ev.v;
        int action = ev.action;
        int timestamp = ev.timestamp;

        // remove self-loops
        if (u == v)
            continue;

        // undirected canonical order
        if (u > v)
            std::swap(u, v);

        std::pair<int, int> e = {u, v};

        if (action == 1) {
            // insertion
            if (inserted[e] == 0) {
                inserted[e] = 1;
                outfile << u << " " << v << " +1 " << timestamp << "\n";
            }
        } else {
            // deletion
            if (inserted[e] == 1) {
                inserted[e] = 0;
                outfile << u << " " << v << " -1 " << timestamp << "\n";
            }
        }
    }

    outfile.close();

    std::cout << "Graph correction completed. Output saved to " << output_file
              << "\n";
    return 0;
}
