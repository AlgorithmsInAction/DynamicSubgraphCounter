#include <bits/stdc++.h>
using namespace std;

static constexpr long long DAY = 86400;
static constexpr long long WEEK = 7 * DAY;
static constexpr long long FORTNIGHT = 14 * DAY;

struct Edge {
    int u, v;
    long long t;
};

struct Event {
    long long t;
    int u, v;
    int delta;
};

struct Interval {
    long long start, end;
};

long long parse_time(const string &s) {
    std::tm tm = {};
    tm.tm_year = stoi(s.substr(0, 4)) - 1900;
    tm.tm_mon = stoi(s.substr(5, 2)) - 1;
    tm.tm_mday = stoi(s.substr(8, 2));
    tm.tm_hour = stoi(s.substr(11, 2));
    tm.tm_min = stoi(s.substr(14, 2));
    tm.tm_sec = stoi(s.substr(17, 2));
    tm.tm_isdst = -1;
    return timegm(&tm);
}

string get_extension(const string &name) {
    size_t pos = name.find_last_of('.');
    if (pos == string::npos)
        return "";
    return name.substr(pos + 1);
}

// reuse containers to avoid realloc
void process_window(const vector<Edge> &edges, long long WINDOW,
                    const string &output_name, vector<Event> &events,
                    unordered_map<long long, Interval> &active) {
    active.clear();
    events.clear();

    active.reserve(edges.size() / 2);
    events.reserve(edges.size() * 2);

    for (const Edge &e : edges) {

        long long key = ((long long)e.u << 32) | e.v;

        auto it = active.find(key);

        if (it == active.end()) {
            active[key] = {e.t, e.t + WINDOW};
        } else {
            if (e.t <= it->second.end)
                it->second.end = max(it->second.end, e.t + WINDOW);
            else {
                events.push_back({it->second.start, e.u, e.v, +1});
                events.push_back({it->second.end, e.u, e.v, -1});
                it->second = {e.t, e.t + WINDOW};
            }
        }
    }

    for (auto &kv : active) {
        int u = kv.first >> 32;
        int v = kv.first & 0xffffffff;
        events.push_back({kv.second.start, u, v, +1});
        events.push_back({kv.second.end, u, v, -1});
    }

    sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        if (a.t != b.t)
            return a.t < b.t;
        return a.delta > b.delta;
    });

    ofstream out(output_name);

    long long step = 0;
    for (const Event &e : events)
        out << e.u << " " << e.v << " " << (e.delta > 0 ? "+1" : "-1") << " "
            << step++ << "\n";

    cerr << "Written " << output_name << " events=" << events.size() << "\n";
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: program input\n";
        return 1;
    }

    string input_name = argv[1];
    ifstream in(input_name);

    if (!in) {
        cerr << "Cannot open input\n";
        return 1;
    }

    string ext = get_extension(input_name);

    string output_base = input_name;
    size_t dot = output_base.find_last_of('.');
    if (dot != string::npos)
        output_base = output_base.substr(0, dot);

    unordered_map<string, int> node_map;
    node_map.reserve(10000000);

    int next_id = 0;

    auto get_id = [&](const string &s) {
        auto it = node_map.find(s);
        if (it != node_map.end())
            return it->second;
        return node_map[s] = next_id++;
    };

    vector<Edge> edges;
    edges.reserve(100000000); // preallocate large

    string line;

    if (ext == "csv" || ext == "tsv")
        getline(in, line);

    while (getline(in, line)) {

        if (line.empty())
            continue;

        Edge e;

        if (ext == "tsv") {

            size_t p1 = line.find('\t');
            size_t p2 = line.find('\t', p1 + 1);
            size_t p3 = line.find('\t', p2 + 1);
            size_t p4 = line.find('\t', p3 + 1);

            string src = line.substr(0, p1);
            string dst = line.substr(p1 + 1, p2 - p1 - 1);
            string time_str = line.substr(p3 + 1, p4 - p3 - 1);

            e.u = get_id(src);
            e.v = get_id(dst);
            e.t = parse_time(time_str);
        } else if (ext == "csv") {

            size_t p1 = line.find(',');
            size_t p2 = line.find(',', p1 + 1);
            size_t p3 = line.find(',', p2 + 1);

            string src = line.substr(0, p1);
            string dst = line.substr(p1 + 1, p2 - p1 - 1);
            string time_str = line.substr(p3 + 1);

            e.u = get_id(src);
            e.v = get_id(dst);
            e.t = stoll(time_str);
        } else {
            stringstream ss(line);
            ss >> e.u >> e.v >> e.t;
        }

        if (e.u > e.v)
            swap(e.u, e.v);

        edges.push_back(e);
    }

    cout << "Edges loaded: " << edges.size() << "\n";
    cout << "Nodes: " << next_id << "\n";

    vector<Event> events;
    unordered_map<long long, Interval> active;

    process_window(edges, DAY, output_base + "_d", events, active);
    process_window(edges, WEEK, output_base + "_w", events, active);
    process_window(edges, FORTNIGHT, output_base + "_f", events, active);
}
