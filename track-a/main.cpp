#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <fcntl.h>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Two exact passes over the mapped log. Pass one discovers each worker's
// endpoints and counts (hour, endpoint) groups; pass two scatters every latency
// straight into its final group slice, so no per-line rows are retained.
constexpr uint32_t HOURS = 72, SLOTS = 8192, CAP = 4096, KEY = 128;
alignas(16) static const char ONES[32] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

static uint64_t load8(const char* p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}
static __m128i load16(const char* p) { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p)); }
static __m256i load32(const char* p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)); }
static uint64_t eq64(const __m256i* v, __m256i a, __m256i b) {
    return uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(v[0], a))) |
           uint64_t(uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(v[1], b)))) << 32;
}

struct Worker {
    const char *begin, *end;
    std::vector<uint16_t> table = std::vector<uint16_t>(SLOTS);  // id + 1, 0 is empty
    std::vector<char> keys = std::vector<char>(size_t{CAP} * KEY);
    std::vector<uint32_t> lens = std::vector<uint32_t>(CAP), rank;
    std::vector<std::string_view> names;
    bool seen[HOURS] = {};
    // Hour-major calloc counters: untouched hours stay unbacked zero pages.
    uint32_t* count = static_cast<uint32_t*>(calloc(size_t{HOURS} * CAP, 4));
    uint32_t* errors = static_cast<uint32_t*>(calloc(size_t{HOURS} * CAP, 4));
    uint16_t* ids = nullptr;  // sized by the shortest possible line; only used pages are backed

    // Exact byte comparison against zero-padded copies of each endpoint.
    uint32_t lookup(const char* s, uint32_t len, const __m256i* v) {
        __m128i head = _mm_and_si128(_mm256_castsi256_si128(v[0]), load16(ONES + 16 - std::min(len, 16u)));
        uint64_t tail = len >= 16 ? load8(s + len - 8) : 0;
        uint64_t h = (_mm_cvtsi128_si64(head) * 0x9E3779B97F4A7C15ull) ^
                     (_mm_cvtsi128_si64(_mm_unpackhi_epi64(head, head)) * 0xC2B2AE3D27D4EB4Full);
        h = (h ^ (tail * 0x165667B19E3779F9ull) ^ len) * 0xD6E8FEB86659FD93ull;
        uint64_t need = len >= 64 ? ~0ull : (1ull << len) - 1;
        for (uint32_t i = static_cast<uint32_t>(h >> 51);; i = (i + 1) & (SLOTS - 1)) {
            uint32_t id = table[i];
            if (!id) {
                id = static_cast<uint32_t>(names.size());
                if (id >= CAP || len > KEY) throw std::runtime_error("unsupported endpoint set");
                table[i] = static_cast<uint16_t>(id + 1);
                memcpy(&keys[size_t{id} * KEY], s, len);
                lens[id] = len;
                names.emplace_back(s, len);
                return id;
            }
            const char* k = &keys[size_t{--id} * KEY];
            if (lens[id] == len && !(~eq64(v, load32(k), load32(k + 32)) & need) && (len <= 64 || !memcmp(k + 64, s + 64, len - 64)))
                return id;
        }
    }

    // Pass one hashes each endpoint and records its local id per line; pass
    // two reuses those ids and lengths, so it only parses hours and latencies.
    template <bool Scatter>
    void parse(uint32_t* values) {
        const __m256i space = _mm256_set1_epi8(' ');
        const __m128i newline = _mm_set1_epi8('\n');
        uint16_t* id = ids;
        for (const char* p = begin; p < end; ++id) {
            // The frozen grammar fixes UTC timestamps and this 72-hour span.
            uint32_t day = (p[8] - '0') * 10 + p[9] - '0' - 30 + (p[6] - '0') * 31;
            uint32_t hour = day * 24 + (p[11] - '0') * 10 + p[12] - '0' - 18;
            uint64_t w = load8(p + 31) ^ 0x2020202020202020ull;  // methods are 3 to 7 bytes
            const char* s = p + 32 + (__builtin_ctzll((w - 0x0101010101010101ull) & ~w & 0x8080808080808080ull) >> 3);
            uint32_t len;
            if constexpr (Scatter) len = lens[*id];
            else {
                __m256i v[2] = {load32(s), load32(s + 32)};
                uint64_t m = eq64(v, space, space);
                len = m ? __builtin_ctzll(m) : 64;
                while (s[len] != ' ') ++len;
                *id = static_cast<uint16_t>(lookup(s, len, v));
            }
            const char* e = s + len;
            uint32_t digits = __builtin_ctz(_mm_movemask_epi8(_mm_cmpeq_epi8(load16(e + 5), newline)));
            size_t slot = size_t{hour} * CAP + *id;
            p = e + 6 + digits;
            if constexpr (Scatter) {
                uint32_t latency = 0;
                if (digits <= 8) {
                    uint64_t d = (load8(e + 5) << (64 - 8 * digits)) & 0x0F0F0F0F0F0F0F0Full;
                    d = (d * 10 + (d >> 8)) & 0x00FF00FF00FF00FFull;
                    d = (d * 100 + (d >> 16)) & 0x0000FFFF0000FFFFull;
                    latency = static_cast<uint32_t>(d * 10000 + (d >> 32));
                } else
                    for (uint32_t i = 0; i < digits; ++i) latency = latency * 10 + static_cast<uint32_t>(e[5 + i] - '0');
                values[count[slot]++] = latency;
            } else {
                ++count[slot];
                errors[slot] += e[1] == '5';
                seen[hour] = true;
            }
        }
    }
};

struct Threads {
    std::vector<std::thread> list;
    template <class F>
    void run(size_t n, F f) {
        for (size_t i = 1; i < n; ++i) list.emplace_back(f, i);
        f(0);
        for (auto& t : list) t.join();
        list.clear();
    }
};

// Quickselect with branch-free Lomuto partitions; the pivot is excluded each
// round, so equal values still shrink the range. Small ranges use insertion sort.
static void select(uint32_t* first, uint32_t* last, uint32_t* k) {
    while (last - first > 24) {
        uint32_t *mid = first + (last - first) / 2, *back = last - 1;
        if (*mid < *first) std::swap(*mid, *first);
        if (*back < *mid) std::swap(*back, *mid);
        if (*mid < *first) std::swap(*mid, *first);
        std::swap(*mid, *back);
        uint32_t pivot = *back, *j = first;
        for (uint32_t* i = first; i < back; ++i) {
            uint32_t x = *i;
            *i = *j;
            *j = x;
            j += x < pivot;
        }
        std::swap(*j, *back);
        if (k == j) return;
        if (k < j) last = j;
        else first = j + 1;
    }
    for (uint32_t* i = first + 1; i < last; ++i)
        for (uint32_t *j = i, x = *i; j > first && j[-1] > x; --j) j[0] = j[-1], j[-1] = x;
}

static const struct Digits {
    char pair[200];
    Digits() {
        for (int i = 0; i < 100; ++i) pair[2 * i] = static_cast<char>('0' + i / 10), pair[2 * i + 1] = static_cast<char>('0' + i % 10);
    }
} digits;

// Formats into a fixed local window and copies it whole; callers keep 32 bytes free.
static char* put(char* o, uint64_t v) {
    char tmp[48];
    char* t = tmp + 24;
    for (; v >= 100; v /= 100) memcpy(t -= 2, digits.pair + 2 * (v % 100), 2);
    if (v >= 10) memcpy(t -= 2, digits.pair + 2 * v, 2);
    else *--t = static_cast<char>('0' + v);
    memcpy(o, t, 24);
    return o + (tmp + 24 - t);
}

// Writes whole 4 KiB blocks with O_DIRECT where the filesystem allows it, so
// the output never sits in this run's page cache; falls back to plain writes.
struct Output {
    int fd = -1;
    bool direct = false;
    char* buf = nullptr;
    size_t used = 0, offset = 0, cap = 1 << 21;
    explicit Output(const std::string& path) {
        fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0644);
        direct = fd >= 0;
        if (!direct) fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0 || posix_memalign(reinterpret_cast<void**>(&buf), 4096, cap)) throw std::runtime_error("cannot create output");
    }
    void emit(size_t n) {
        for (size_t done = 0; done < n;) {
            ssize_t k = pwrite(fd, buf + done, n - done, static_cast<off_t>(offset + done));
            if (k < 0 && direct && done == 0) {  // O_DIRECT refused: switch to buffered
                fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_DIRECT);
                direct = false;
                continue;
            }
            if (k <= 0) throw std::runtime_error("write failed");
            done += static_cast<size_t>(k);
        }
        offset += n;
    }
    void append(const char* p, size_t n) {
        while (n) {
            size_t k = std::min(n, cap - used);
            memcpy(buf + used, p, k);
            used += k, p += k, n -= k;
            size_t whole = used & ~size_t{4095};
            if (used == cap || (whole && used - whole < 4096 && whole >= (1 << 20))) {
                emit(whole);
                memmove(buf, buf + whole, used - whole);
                used -= whole;
            }
        }
    }
    void finish() {
        size_t size = offset + used, padded = (used + 4095) & ~size_t{4095};
        memset(buf + used, 0, padded - used);
        emit(direct ? padded : used);
        if (ftruncate(fd, static_cast<off_t>(size)) || close(fd)) throw std::runtime_error("close failed");
    }
};

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        cpu_set_t set;
        size_t workers = sched_getaffinity(0, sizeof set, &set) == 0 ? CPU_COUNT(&set) : 12;
        for (int i = 1; i + 1 < argc; i += 2) {
            std::string arg = argv[i];
            if (arg == "--data") data_dir = argv[i + 1];
            else if (arg == "--out") out_dir = argv[i + 1];
            else if (arg == "--workers") workers = std::stoul(argv[i + 1]);
        }
        workers = std::clamp<size_t>(workers, 1, 64);
        int fd = open((data_dir + "/requests.log").c_str(), O_RDONLY);
        struct stat st{};
        if (fd < 0 || fstat(fd, &st) != 0) throw std::runtime_error("cannot open requests.log");
        size_t size = static_cast<size_t>(st.st_size);
        Output out(out_dir + "/summary.txt");
        if (!size) return out.finish(), 0;
        // Vector loads may read past the last line: back the mapping with
        // extra anonymous zero pages.
        void* base = mmap(nullptr, size + 8192, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED || mmap(base, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED)
            throw std::runtime_error("mmap failed");
        const char* data = static_cast<const char*>(base);
        std::vector<Worker> part(workers);
        for (size_t i = 0; i < workers; ++i) {
            size_t a = size * i / workers;
            while (a > 0 && data[a - 1] != '\n') ++a;
            part[i].begin = data + a;
            if (i) part[i - 1].end = data + a;
        }
        part.back().end = data + size;
        for (auto& w : part) w.ids = static_cast<uint16_t*>(malloc((w.end - w.begin) / 40 * 2 + 2));
        Threads threads;
        threads.run(workers, [&](size_t i) { part[i].parse<false>(nullptr); });
        // Exact global byte order of every endpoint string, prefixes first.
        std::vector<std::string_view> names;
        for (auto& w : part) names.insert(names.end(), w.names.begin(), w.names.end());
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        const size_t E = names.size();
        std::vector<uint32_t> total(HOURS * E), errors(HOURS * E), start(HOURS * E + 1);
        for (auto& w : part) {
            w.rank.resize(w.names.size());
            for (size_t j = 0; j < w.names.size(); ++j)
                w.rank[j] = static_cast<uint32_t>(std::lower_bound(names.begin(), names.end(), w.names[j]) - names.begin());
            for (uint32_t h = 0; h < HOURS; ++h)
                if (w.seen[h])
                    for (size_t j = 0; j < w.names.size(); ++j) {
                        total[h * E + w.rank[j]] += w.count[size_t{h} * CAP + j];
                        errors[h * E + w.rank[j]] += w.errors[size_t{h} * CAP + j];
                    }
            free(w.errors);
        }
        for (size_t g = 0; g < HOURS * E; ++g) start[g + 1] = start[g] + total[g];
        // Each worker owns a disjoint slice of every group; counters become cursors.
        std::vector<uint32_t> cursor(start.begin(), start.end() - 1);
        for (auto& w : part)
            for (uint32_t h = 0; h < HOURS; ++h)
                if (w.seen[h])
                    for (size_t j = 0; j < w.names.size(); ++j)
                        if (uint32_t& n = w.count[size_t{h} * CAP + j]) {
                            uint32_t at = cursor[h * E + w.rank[j]];
                            cursor[h * E + w.rank[j]] += n;
                            n = at;
                        }
        std::vector<uint32_t>().swap(cursor);
        uint32_t* values = new uint32_t[start.back() + 1];
        threads.run(workers, [&](size_t i) { part[i].parse<true>(values); });
        part.clear();
        // Hours are claimed in order, formatted privately and appended in order.
        std::atomic<uint32_t> next{0}, written{0};
        threads.run(workers, [&](size_t) {
            std::string buf;
            for (uint32_t h; (h = next.fetch_add(1)) < HOURS;) {
                buf.resize(std::max<size_t>(buf.size(), 1024));
                size_t used = 0;
                uint32_t day = (h + 18) / 24, hh = (h + 18) % 24;
                char stamp[22] = "2026-10-30T00:00:00Z ";
                if (day >= 2) stamp[6] = '1', stamp[8] = '0';
                stamp[9] = static_cast<char>('0' + (day < 2 ? day : day - 1));
                stamp[11] = static_cast<char>('0' + hh / 10);
                stamp[12] = static_cast<char>('0' + hh % 10);
                for (size_t r = 0; r < E; ++r) {
                    size_t g = h * E + r;
                    uint64_t n = total[g];
                    if (!n) continue;
                    if (buf.size() - used < 300) buf.resize(buf.size() * 2);
                    uint32_t* first = values + start[g];
                    uint32_t* last = first + n;
                    char* o = buf.data() + used;
                    memcpy(o, stamp, 21);
                    o += 21;
                    memcpy(o, names[r].data(), names[r].size());
                    o += names[r].size();
                    *o++ = ' ';
                    o = put(o, n);
                    uint32_t* rest = first;
                    for (uint64_t p : {50, 95, 99}) {
                        uint32_t* at = first + (p * n + 99) / 100 - 1;
                        if (at >= rest) {
                            select(rest, last, at);
                            rest = at + 1;
                        }
                        *o++ = ' ';
                        o = put(o, *at);
                    }
                    *o++ = ' ';
                    o = put(o, errors[g]);
                    *o++ = ' ';
                    o = put(o, errors[g] * uint64_t{1000000} / n);
                    *o++ = '\n';
                    used = static_cast<size_t>(o - buf.data());
                }
                for (uint32_t w; (w = written.load()) != h;) written.wait(w);
                out.append(buf.data(), used);
                written.store(h + 1);
                written.notify_all();
            }
        });
        out.finish();
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
