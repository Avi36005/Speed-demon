#include <algorithm>
#include <atomic>
#include <thread>
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
struct Graph {
    std::vector<uint32_t> offsets, reverse_offsets;
    std::vector<Edge> edges, reverse_edges;
    explicit Graph(const std::string& path) {
        Input input(path);
        Parser parser{input.data, input.data + input.size};
        uint32_t n = parser.number(), m = parser.number();
        const char* start = parser.p;
        offsets.resize(size_t{n} + 1, 0);
        reverse_offsets.resize(size_t{n} + 1, 0);
        for (uint32_t i = 0; i < m; ++i) {
            uint32_t u = parser.number(), v = parser.number();
            parser.number();
            ++offsets[u + 1];
            ++reverse_offsets[v + 1];
        }
        std::partial_sum(offsets.begin(), offsets.end(), offsets.begin());
        std::partial_sum(reverse_offsets.begin(), reverse_offsets.end(), reverse_offsets.begin());
        auto cursor = offsets, reverse_cursor = reverse_offsets;
        edges.resize(m);
        reverse_edges.resize(m);
        parser.p = start;
        for (uint32_t i = 0; i < m; ++i) {
            uint32_t u = parser.number(), v = parser.number(), w = parser.number();
            edges[cursor[u]++] = {v, w};
            reverse_edges[reverse_cursor[v]++] = {u, w};
        }
    }
};

struct State {
    static constexpr uint64_t infinity = std::numeric_limits<uint64_t>::max();
    std::vector<uint64_t> forward, backward;
    std::vector<uint32_t> touched_forward, touched_backward;
    explicit State(size_t n) : forward(n, infinity), backward(n, infinity) {}
    void reset() {
        for (uint32_t u : touched_forward) forward[u] = infinity;
        for (uint32_t u : touched_backward) backward[u] = infinity;
        touched_forward.clear();
        touched_backward.clear();
    }
};

int64_t shortest_path(const Graph& graph, uint32_t source, uint32_t target, State& state) {
    constexpr uint64_t infinity = State::infinity;
    if (source == target) return 0;
    if (graph.offsets[source] == graph.offsets[source + 1] ||
        graph.reverse_offsets[target] == graph.reverse_offsets[target + 1]) return -1;
    state.reset();
    using Item = std::pair<uint64_t, uint32_t>;
    using Heap = std::priority_queue<Item, std::vector<Item>, std::greater<Item>>;
    Heap forward_heap, backward_heap;
    state.forward[source] = 0;
    state.backward[target] = 0;
    state.touched_forward.push_back(source);
    state.touched_backward.push_back(target);
    forward_heap.emplace(0, source);
    backward_heap.emplace(0, target);
    uint64_t best = infinity;
    auto connect = [&](uint64_t a, uint64_t b) {
        if (a != infinity && b != infinity && a <= infinity - b)
            best = std::min(best, a + b);
    };
    while (true) {
        while (!forward_heap.empty() && forward_heap.top().first != state.forward[forward_heap.top().second])
            forward_heap.pop();
        while (!backward_heap.empty() && backward_heap.top().first != state.backward[backward_heap.top().second])
            backward_heap.pop();
        if (forward_heap.empty() || backward_heap.empty()) break;
        uint64_t lower_forward = forward_heap.top().first;
        uint64_t lower_backward = backward_heap.top().first;
        // A first meeting is only an upper bound. Stop only when the valid
        // frontier minima cannot improve it, with subtraction avoiding overflow.
        if (best != infinity && (lower_forward >= best || lower_backward >= best - lower_forward)) break;
        // Frontier size balances work even when one direction begins with a
        // UINT32_MAX-weight edge. Correctness does not depend on this choice.
        bool reverse = backward_heap.size() < forward_heap.size();
        auto& heap = reverse ? backward_heap : forward_heap;
        auto& distance = reverse ? state.backward : state.forward;
        const auto& other = reverse ? state.forward : state.backward;
        auto& touched = reverse ? state.touched_backward : state.touched_forward;
        const auto& offsets = reverse ? graph.reverse_offsets : graph.offsets;
        const auto& edges = reverse ? graph.reverse_edges : graph.edges;
        auto [d, u] = heap.top();
        heap.pop();
        connect(d, other[u]);
        for (uint32_t e = offsets[u]; e < offsets[u + 1]; ++e) {
            auto edge = edges[e];
            if (d > infinity - edge.weight) continue;
            uint64_t next = d + edge.weight;
            if (next < distance[edge.to]) {
                if (distance[edge.to] == infinity) touched.push_back(edge.to);
                distance[edge.to] = next;
                heap.emplace(next, edge.to);
                connect(next, other[edge.to]);
            }
        }
    }
    return best == infinity ? -1 : static_cast<int64_t>(best);
}

struct Threads {
    std::vector<std::thread> values;
    void join() { for (auto& thread : values) if (thread.joinable()) thread.join(); }
    ~Threads() { join(); }
};

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        size_t worker_count = 4;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (i + 1 < argc && arg == "--data") data_dir = argv[++i];
            else if (i + 1 < argc && arg == "--out") out_dir = argv[++i];
            else if (i + 1 < argc && arg == "--workers") worker_count = std::stoul(argv[++i]);
            else throw std::runtime_error("usage: solution [--data DIR] [--out DIR] [--workers 1..12]");
        }
        if (!worker_count || worker_count > 12) throw std::runtime_error("workers must be 1..12");
        Graph graph(data_dir + "/graph.txt");
        Input queries(data_dir + "/queries.txt");
        Parser parser{queries.data, queries.data + queries.size};
        std::vector<std::pair<uint32_t, uint32_t>> pairs;
        while (parser.p < parser.end) {
            uint32_t source = parser.number(), target = parser.number();
            pairs.emplace_back(source, target);
        }
        std::vector<int64_t> answers(pairs.size());
        std::atomic<size_t> next_query{0};
        auto search = [&] {
            State state(graph.offsets.size() - 1);
            for (;;) {
                size_t q = next_query.fetch_add(1, std::memory_order_relaxed);
                if (q >= pairs.size()) break;
                answers[q] = shortest_path(graph, pairs[q].first, pairs[q].second, state);
            }
        };
        Threads threads;
        for (size_t i = 1; i < worker_count; ++i) threads.values.emplace_back(search);
        search();
        threads.join();
        std::ofstream out(out_dir + "/distances.txt", std::ios::binary);
        for (auto distance : answers) out << distance << '\n';
        if (!out) throw std::runtime_error("output write failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
