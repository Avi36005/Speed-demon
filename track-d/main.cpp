#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <queue>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <vector>

// Exact bidirectional Dijkstra over forward and reverse CSR graphs, built by
// two parallel parses (atomic degree counts, then atomic slot claims).
constexpr uint64_t INF = UINT64_MAX;

static const char* map_file(const std::string& path, size_t& size) {
    int fd = open(path.c_str(), O_RDONLY);
    struct stat st{};
    if (fd < 0 || fstat(fd, &st) != 0) throw std::runtime_error("cannot open " + path);
    size = static_cast<size_t>(st.st_size);
    void* base = mmap(nullptr, size + 8192, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED || (size && mmap(base, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED))
        throw std::runtime_error("mmap failed");
    close(fd);
    return static_cast<const char*>(base);
}
// Parses one decimal number (up to 8 digits at once) and skips its separator.
static uint32_t number(const char*& p) {
    uint64_t v;
    memcpy(&v, p, 8);
    uint64_t x = v ^ 0x3030303030303030ull, stop = ((x + 0x7676767676767676ull) | x) & 0x8080808080808080ull;
    if (!stop) {
        uint64_t n = 0;
        while (static_cast<unsigned char>(*p - '0') < 10) n = n * 10 + static_cast<uint64_t>(*p++ - '0');
        ++p;
        return static_cast<uint32_t>(n);
    }
    uint32_t len = static_cast<uint32_t>(__builtin_ctzll(stop)) >> 3;
    p += len + 1;
    x = (x << (64 - 8 * len)) & 0x0F0F0F0F0F0F0F0Full;
    x = (x * 10 + (x >> 8)) & 0x00FF00FF00FF00FFull;
    x = (x * 100 + (x >> 16)) & 0x0000FFFF0000FFFFull;
    return static_cast<uint32_t>(x * 10000 + (x >> 32));
}

struct Edge {
    uint32_t to, weight;
};

// 32-bit distances per node; the rare values from 2^32 - 2 up are kept
// exactly in a side map, so every comparison still sees the full value.
constexpr uint32_t BIG = UINT32_MAX - 1;
struct Dist {
    std::vector<uint32_t> small;
    std::unordered_map<uint32_t, uint64_t> big;
    explicit Dist(uint32_t n) : small(n, UINT32_MAX) {}
    uint64_t get(uint32_t v) const {
        uint32_t x = small[v];
        return x < BIG ? x : x == UINT32_MAX ? INF : big.at(v);
    }
    void set(uint32_t v, uint64_t d) {
        if (d < BIG) small[v] = static_cast<uint32_t>(d);
        else small[v] = BIG, big[v] = d;
    }
};

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        cpu_set_t cpus;
        size_t workers = sched_getaffinity(0, sizeof cpus, &cpus) == 0 ? CPU_COUNT(&cpus) : 12;
        for (int i = 1; i + 1 < argc; i += 2) {
            std::string arg = argv[i];
            if (arg == "--data") data_dir = argv[i + 1];
            else if (arg == "--out") out_dir = argv[i + 1];
            else if (arg == "--workers") workers = std::stoul(argv[i + 1]);
        }
        workers = std::clamp<size_t>(workers, 1, 64);
        std::vector<std::thread> list;
        auto run = [&](auto f) {
            for (size_t i = 1; i < workers; ++i) list.emplace_back(f, i);
            f(0);
            for (auto& t : list) t.join();
            list.clear();
        };
        size_t gsize, qsize;
        const char* graph = map_file(data_dir + "/graph.txt", gsize);
        const char* qtext = map_file(data_dir + "/queries.txt", qsize);
        const char* p = graph;
        const uint32_t n = number(p);
        number(p);
        const char* body = p;
        std::vector<const char*> cut(workers + 1, graph + gsize);
        cut[0] = body;
        for (size_t w = 1; w < workers; ++w) {
            const char* c = std::max(cut[w - 1], body + (graph + gsize - body) * w / workers);
            while (c < graph + gsize && c[-1] != '\n') ++c;
            cut[w] = c;
        }
        // Degrees, then offsets; the degree arrays then serve as slot cursors.
        std::vector<uint32_t> out_at(size_t{n} + 1), in_at(size_t{n} + 1);
        auto edges_of = [&](size_t w, auto f) {
            for (const char* q = cut[w]; q < cut[w + 1];) {
                uint32_t u = number(q), v = number(q), wt = number(q);
                if (u != v) f(u, v, wt);  // a self-loop never shortens a path
            }
        };
        run([&](size_t w) {
            edges_of(w, [&](uint32_t u, uint32_t v, uint32_t) {
                std::atomic_ref<uint32_t>(out_at[u]).fetch_add(1, std::memory_order_relaxed);
                std::atomic_ref<uint32_t>(in_at[v]).fetch_add(1, std::memory_order_relaxed);
            });
        });
        std::vector<uint32_t> out_off(size_t{n} + 1), in_off(size_t{n} + 1);
        for (uint32_t i = 0; i < n; ++i) out_off[i + 1] = out_off[i] + out_at[i], in_off[i + 1] = in_off[i] + in_at[i];
        std::copy(out_off.begin(), out_off.end(), out_at.begin());
        std::copy(in_off.begin(), in_off.end(), in_at.begin());
        std::vector<Edge> out_edges(out_off[n]), in_edges(in_off[n]);
        run([&](size_t w) {
            edges_of(w, [&](uint32_t u, uint32_t v, uint32_t wt) {
                out_edges[std::atomic_ref<uint32_t>(out_at[u]).fetch_add(1, std::memory_order_relaxed)] = {v, wt};
                in_edges[std::atomic_ref<uint32_t>(in_at[v]).fetch_add(1, std::memory_order_relaxed)] = {u, wt};
            });
        });
        std::vector<uint32_t>().swap(out_at), std::vector<uint32_t>().swap(in_at);

        std::vector<std::pair<uint32_t, uint32_t>> queries;
        for (const char* q = qtext; q < qtext + qsize;) {
            uint32_t s = number(q), t = number(q);
            queries.emplace_back(s, t);
        }
        std::vector<int64_t> answers(queries.size());
        std::atomic<size_t> next{0};
        run([&](size_t) {
            Dist dist[2] = {Dist(n), Dist(n)};
            std::vector<uint32_t> touched[2];
            using Item = std::pair<uint64_t, uint32_t>;
            std::vector<Item> heap[2];
            auto later = [](const Item& a, const Item& b) { return a.first > b.first; };
            for (size_t qi; (qi = next.fetch_add(1)) < queries.size();) {
                auto [s, t] = queries[qi];
                if (s == t) continue;
                if (out_off[s] == out_off[s + 1] || in_off[t] == in_off[t + 1]) {
                    answers[qi] = -1;
                    continue;
                }
                for (int d = 0; d < 2; ++d) {
                    for (uint32_t u : touched[d]) dist[d].small[u] = UINT32_MAX;
                    dist[d].big.clear();
                    touched[d].clear();
                    heap[d].clear();
                }
                dist[0].set(s, 0), dist[1].set(t, 0);
                touched[0].push_back(s), touched[1].push_back(t);
                heap[0].push_back({0, s}), heap[1].push_back({0, t});
                uint64_t best = INF;
                for (;;) {
                    for (int d = 0; d < 2; ++d)
                        while (!heap[d].empty() && heap[d].front().first != dist[d].get(heap[d].front().second))
                            std::pop_heap(heap[d].begin(), heap[d].end(), later), heap[d].pop_back();
                    if (heap[0].empty() || heap[1].empty()) break;
                    uint64_t lf = heap[0].front().first, lb = heap[1].front().first;
                    // A meeting is only an upper bound; stop once no frontier can improve it.
                    if (best != INF && (lf >= best || lb >= best - lf)) break;
                    int d = heap[1].size() < heap[0].size();
                    auto [du, u] = heap[d].front();
                    std::pop_heap(heap[d].begin(), heap[d].end(), later), heap[d].pop_back();
                    const auto& other = dist[1 - d];
                    if (uint64_t o = other.get(u); o != INF) best = std::min(best, du + o);
                    const Edge* e = d ? in_edges.data() + in_off[u] : out_edges.data() + out_off[u];
                    const Edge* end = d ? in_edges.data() + in_off[u + 1] : out_edges.data() + out_off[u + 1];
                    for (; e < end; ++e) {
                        uint64_t nd = du + e->weight;  // at most (n - 1) * (2^32 - 1): no overflow
                        uint64_t cur = dist[d].get(e->to);
                        if (nd < cur) {
                            if (cur == INF) touched[d].push_back(e->to);
                            dist[d].set(e->to, nd);
                            heap[d].push_back({nd, e->to});
                            std::push_heap(heap[d].begin(), heap[d].end(), later);
                            if (uint64_t o = other.get(e->to); o != INF) best = std::min(best, nd + o);
                        }
                    }
                }
                answers[qi] = best == INF ? -1 : static_cast<int64_t>(best);
            }
        });
        std::string out;
        char buf[24];
        for (int64_t a : answers) out.append(buf, static_cast<size_t>(snprintf(buf, sizeof buf, "%lld\n", static_cast<long long>(a))));
        FILE* f = fopen((out_dir + "/distances.txt").c_str(), "wb");
        if (!f || fwrite(out.data(), 1, out.size(), f) != out.size() || fclose(f)) throw std::runtime_error("write failed");
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
