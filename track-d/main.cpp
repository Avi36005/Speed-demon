#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

struct Input {
    const char* data = "";
    size_t size = 0;
    explicit Input(const std::string& path) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) throw std::runtime_error("cannot open " + path);
        struct stat st{};
        if (fstat(fd, &st) != 0) { close(fd); throw std::runtime_error("fstat failed"); }
        size = static_cast<size_t>(st.st_size);
        if (size) {
            void* mapping = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            close(fd);
            if (mapping == MAP_FAILED) throw std::runtime_error("mmap failed");
            data = static_cast<const char*>(mapping);
        } else close(fd);
    }
    ~Input() { if (size) munmap(const_cast<char*>(data), size); }
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
};

struct Parser {
    const char* p;
    const char* end;
    uint32_t number() {
        uint32_t value = 0;
        while (p < end && *p >= '0' && *p <= '9')
            value = value * 10 + static_cast<unsigned>(*p++ - '0');
        if (p < end) ++p;  // one space or LF, as specified
        return value;
    }
};

struct Edge { uint32_t to, weight; };
struct Arc { uint32_t from, to, weight; };
struct Graph {
    std::vector<uint32_t> offsets;
    std::vector<Edge> edges;
    explicit Graph(const std::string& path) {
        Input input(path);
        Parser parser{input.data, input.data + input.size};
        uint32_t n = parser.number(), m = parser.number();
        offsets.resize(size_t{n} + 1, 0);
        std::vector<Arc> arcs(m);
        for (auto& arc : arcs) {
            arc = {parser.number(), parser.number(), parser.number()};
            ++offsets[arc.from + 1];
        }
        std::partial_sum(offsets.begin(), offsets.end(), offsets.begin());
        auto cursor = offsets;
        edges.resize(m);
        for (auto arc : arcs) edges[cursor[arc.from]++] = {arc.to, arc.weight};
        // Every directed edge survives, including parallel edges and loops.
        // The construction buffers and mapping are released before searches.
    }
};

int64_t shortest_path(const Graph& graph, uint32_t source, uint32_t target,
                      std::vector<uint64_t>& distance) {
    constexpr uint64_t infinity = std::numeric_limits<uint64_t>::max();
    std::fill(distance.begin(), distance.end(), infinity);
    using Item = std::pair<uint64_t, uint32_t>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;
    distance[source] = 0;
    heap.emplace(0, source);
    while (!heap.empty()) {
        auto [d, u] = heap.top();
        heap.pop();
        if (d != distance[u]) continue;
        if (u == target) return static_cast<int64_t>(d);
        for (uint32_t e = graph.offsets[u]; e < graph.offsets[u + 1]; ++e) {
            auto edge = graph.edges[e];
            if (d > infinity - edge.weight) continue;
            uint64_t next = d + edge.weight;
            if (next < distance[edge.to]) {
                distance[edge.to] = next;
                heap.emplace(next, edge.to);
            }
        }
    }
    return -1;
}

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (i + 1 < argc && arg == "--data") data_dir = argv[++i];
            else if (i + 1 < argc && arg == "--out") out_dir = argv[++i];
            else throw std::runtime_error("usage: solution [--data DIR] [--out DIR]");
        }
        Graph graph(data_dir + "/graph.txt");
        Input queries(data_dir + "/queries.txt");
        Parser parser{queries.data, queries.data + queries.size};
        std::vector<uint64_t> distance(graph.offsets.size() - 1);
        std::ofstream out(out_dir + "/distances.txt", std::ios::binary);
        while (parser.p < parser.end) {
            uint32_t source = parser.number(), target = parser.number();
            out << shortest_path(graph, source, target, distance) << '\n';
        }
        if (!out) throw std::runtime_error("output write failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
