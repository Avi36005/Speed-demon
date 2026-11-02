#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <immintrin.h>
#include <memory>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

// One parse files each line as a 32-bit record (endpoint id, error bit,
// latency) under its hour. Hours are then grouped and selected in cache,
// leaving compact results and exact text sizes; the records are freed before
// the output is formatted in parallel straight into the mapped output file.
constexpr uint32_t HOURS = 72, SLOTS = 8192, CAP = 4096, KEY = 64, CHUNK = 1024, ESC = (1u << 19) - 1;
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
static void drop(const char* a, const char* b) {  // parallel teardown of a mapping slice
    uintptr_t s = (reinterpret_cast<uintptr_t>(a) + 4095) & ~uintptr_t{4095}, e = reinterpret_cast<uintptr_t>(b) & ~uintptr_t{4095};
    if (s < e) madvise(reinterpret_cast<void*>(s), e - s, MADV_DONTNEED);
}

// One endpoint table for every thread. A slot holds a hash tag and id + 1;
// the id's zero-padded 64-byte key, length and full copy are written before
// the slot is published with a release CAS, and confirmed by exact compares.
struct Names {
    std::atomic<uint64_t> slots[SLOTS] = {};
    alignas(32) char keys[CAP][KEY] = {};
    uint32_t lens[CAP] = {};
    const char* full[CAP] = {};
    std::atomic<uint32_t> next{0};

    uint32_t lookup(const char* s, uint32_t len, const __m256i* v) {
        __m128i head = _mm_and_si128(_mm256_castsi256_si128(v[0]), load16(ONES + 16 - std::min(len, 16u)));
        uint64_t tail = len >= 16 ? load8(s + len - 8) : 0;
        uint64_t h = (_mm_cvtsi128_si64(head) * 0x9E3779B97F4A7C15ull) ^
                     (_mm_cvtsi128_si64(_mm_unpackhi_epi64(head, head)) * 0xC2B2AE3D27D4EB4Full);
        h = (h ^ (tail * 0x165667B19E3779F9ull) ^ len) * 0xD6E8FEB86659FD93ull;
        uint64_t need = len >= 64 ? ~0ull : (1ull << len) - 1, tag = h >> 32 << 32;
        for (uint32_t i = static_cast<uint32_t>(h >> 19) & (SLOTS - 1);; i = (i + 1) & (SLOTS - 1)) {
            uint64_t e = slots[i].load(std::memory_order_acquire);
            if (!e) {
                uint32_t id = next.fetch_add(1, std::memory_order_relaxed);
                if (id >= CAP) throw std::runtime_error("unsupported endpoint set");
                memcpy(keys[id], s, std::min(len, KEY));
                lens[id] = len;
                full[id] = static_cast<const char*>(memcpy(new char[len + 1], s, len));
                if (slots[i].compare_exchange_strong(e, tag | (id + 1), std::memory_order_acq_rel)) return id;
            }
            uint32_t id = static_cast<uint32_t>(e) - 1;
            if ((e & ~0xFFFFFFFFull) == tag && lens[id] == len && !(~eq64(v, load32(keys[id]), load32(keys[id] + 32)) & need) &&
                (len <= KEY || !memcmp(full[id] + KEY, s + KEY, len - KEY)))
                return id;
        }
    }
};

// The frozen grammar fixes UTC timestamps and this 72-hour span.
static uint32_t hour_of(const char* p) {
    uint32_t day = (p[8] - '0') * 10 + p[9] - '0' - 30 + (p[6] - '0') * 31;
    return day * 24 + (p[11] - '0') * 10 + p[12] - '0' - 18;
}

// Each thread's records for one hour: 4 KiB chunks from a private pool.
struct Bucket {
    std::vector<uint32_t*> chunks;
    uint32_t *cur = nullptr, *end = nullptr;
    size_t size() const { return chunks.empty() ? 0 : (chunks.size() - 1) * CHUNK + static_cast<size_t>(cur - chunks.back()); }
};
struct Worker {
    Bucket hours[HOURS];
    std::vector<std::pair<uint32_t*, size_t>> pools;
    uint32_t *pool = nullptr, *pool_end = nullptr;
    void push(Bucket& b, uint32_t x) {
        if (b.cur == b.end) {
            if (pool == pool_end) {
                size_t n = size_t{1} << 22;  // 16 MiB of address space; only used pages are backed
                pool = static_cast<uint32_t*>(mmap(nullptr, n * 4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0));
                if (pool == MAP_FAILED) throw std::runtime_error("mmap failed");
                pools.emplace_back(pool, n * 4);
                pool_end = pool + n;
            }
            b.chunks.push_back(b.cur = pool);
            b.end = pool += CHUNK;
        }
        *b.cur++ = x;
    }
    template <class F>
    void each(uint32_t h, F f) const {  // every record of hour h, in order
        const Bucket& b = hours[h];
        for (size_t c = 0; c < b.chunks.size(); ++c)
            for (const uint32_t *p = b.chunks[c], *e = c + 1 < b.chunks.size() ? p + CHUNK : b.cur; p < e; ++p) f(*p);
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
    for (uint32_t* i = first + 1; i < last; ++i) {
        uint32_t x = *i, *j = i;
        for (; j > first && j[-1] > x; --j) j[0] = j[-1];
        *j = x;
    }
}

static const struct Digits {
    char pair[200];
    Digits() {
        for (int i = 0; i < 100; ++i) pair[2 * i] = static_cast<char>('0' + i / 10), pair[2 * i + 1] = static_cast<char>('0' + i % 10);
    }
} digits;
static uint32_t width(uint64_t v) {
    uint32_t w = 1;
    for (; v >= 10; v /= 10) ++w;
    return w;
}
// Writes exactly width(v) bytes, backwards from o + width(v).
static char* put(char* o, uint64_t v) {
    char *e = o + width(v), *t = e;
    for (; v >= 100; v /= 100) memcpy(t -= 2, digits.pair + 2 * (v % 100), 2);
    if (v >= 10) memcpy(t - 2, digits.pair + 2 * v, 2);
    else t[-1] = static_cast<char>('0' + v);
    return e;
}

struct Group {
    uint32_t id, n, p50, p95, p99, errors;
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
        int out = open((out_dir + "/summary.txt").c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (out < 0) throw std::runtime_error("cannot create summary.txt");
        if (!size) return close(out);
        // Vector loads may read past the last line: back the mapping with
        // extra anonymous zero pages.
        void* base = mmap(nullptr, size + 8192, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED || mmap(base, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED)
            throw std::runtime_error("mmap failed");
        const char* data = static_cast<const char*>(base);
        // Pieces start on line boundaries; threads claim them in turn.
        const size_t pieces = workers * 8;
        std::vector<size_t> cut(pieces + 1, size);
        cut[0] = 0;
        for (size_t i = 1; i < pieces; ++i) {
            size_t a = std::max(cut[i - 1], size * i / pieces);
            while (a > 0 && a < size && data[a - 1] != '\n') ++a;
            cut[i] = a;
        }
        auto* names = new Names;
        std::vector<Worker> part(workers);
        std::atomic<size_t> claim{0};
        Threads threads;
        threads.run(workers, [&](size_t w) {
            Worker& me = part[w];
            const __m256i space = _mm256_set1_epi8(' ');
            const __m128i newline = _mm_set1_epi8('\n');
            for (size_t c; (c = claim.fetch_add(1)) < pieces;)
                for (const char *p = data + cut[c], *end = data + cut[c + 1]; p < end;) {
                    uint32_t hour = hour_of(p);
                    uint64_t m = load8(p + 31) ^ 0x2020202020202020ull;  // methods are 3 to 7 bytes
                    const char* s = p + 32 + (__builtin_ctzll((m - 0x0101010101010101ull) & ~m & 0x8080808080808080ull) >> 3);
                    __m256i v[2] = {load32(s), load32(s + 32)};
                    uint64_t sp = eq64(v, space, space);
                    uint32_t len = sp ? static_cast<uint32_t>(__builtin_ctzll(sp)) : 64;
                    while (s[len] != ' ') ++len;
                    uint32_t id = names->lookup(s, len, v);
                    const char* e = s + len;
                    uint32_t digits = static_cast<uint32_t>(__builtin_ctz(_mm_movemask_epi8(_mm_cmpeq_epi8(load16(e + 5), newline))));
                    uint32_t latency = 0;
                    if (digits <= 8) {
                        uint64_t d = (load8(e + 5) << (64 - 8 * digits)) & 0x0F0F0F0F0F0F0F0Full;
                        d = (d * 10 + (d >> 8)) & 0x00FF00FF00FF00FFull;
                        d = (d * 100 + (d >> 16)) & 0x0000FFFF0000FFFFull;
                        latency = static_cast<uint32_t>(d * 10000 + (d >> 32));
                    } else
                        for (uint32_t i = 0; i < digits; ++i) latency = latency * 10 + static_cast<uint32_t>(e[5 + i] - '0');
                    Bucket& b = me.hours[hour];
                    me.push(b, id << 20 | uint32_t{e[1] == '5'} << 19 | std::min(latency, ESC));
                    if (latency >= ESC) me.push(b, latency);  // escape: the full value follows
                    p = e + 6 + digits;
                }
        });
        // Exact byte order of every endpoint (prefixes first).
        const uint32_t E = std::min(names->next.load(), CAP);
        std::vector<uint32_t> order(E);
        for (uint32_t i = 0; i < E; ++i) order[i] = i;
        auto name = [&](uint32_t id) { return std::string_view(names->full[id], names->lens[id]); };
        std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return name(a) < name(b); });
        size_t most = 0;
        for (uint32_t h = 0; h < HOURS; ++h) {
            size_t n = 0;
            for (auto& w : part) n += w.hours[h].size();
            most = std::max(most, n);
        }
        std::vector<std::vector<Group>> groups(HOURS);
        std::vector<size_t> bytes(HOURS + 1);
        std::atomic<uint32_t> next_hour{0};
        threads.run(workers, [&](size_t w) {
            drop(data + size * w / workers, data + size * (w + 1) / workers);
            std::vector<uint32_t> count(CAP), errors(CAP), fill(CAP);
            std::unique_ptr<uint32_t[]> values(new uint32_t[most + 1]);  // pages backed only as used
            for (uint32_t h; (h = next_hour.fetch_add(1)) < HOURS;) {
                for (auto& p : part) {
                    bool escaped = false;
                    p.each(h, [&](uint32_t x) {
                        if (escaped) return void(escaped = false);
                        ++count[x >> 20], errors[x >> 20] += x >> 19 & 1;
                        escaped = (x & ESC) == ESC;
                    });
                }
                uint32_t at = 0;
                for (uint32_t id : order) fill[id] = at, at += count[id];
                for (auto& p : part) {
                    uint32_t pending = UINT32_MAX;
                    p.each(h, [&](uint32_t x) {
                        if (pending != UINT32_MAX) return void((values[fill[pending]++] = x, pending = UINT32_MAX));
                        if ((x & ESC) == ESC) pending = x >> 20;
                        else values[fill[x >> 20]++] = x & ESC;
                    });
                }
                size_t text = 0;
                auto& out_groups = groups[h];
                for (uint32_t id : order) {
                    uint64_t n = count[id];
                    if (!n) continue;
                    uint32_t *first = values.get() + fill[id] - n, *last = first + n, *rest = first, ranks[3];
                    for (int i = 0; i < 3; ++i) {
                        uint32_t* rank = first + (uint64_t{i == 0 ? 50u : i == 1 ? 95u : 99u} * n + 99) / 100 - 1;
                        if (rank >= rest) select(rest, last, rank), rest = rank + 1;
                        ranks[i] = *rank;
                    }
                    Group g{id, static_cast<uint32_t>(n), ranks[0], ranks[1], ranks[2], errors[id]};
                    text += 21 + names->lens[id] + 7 + width(g.n) + width(g.p50) + width(g.p95) + width(g.p99) + width(g.errors) +
                            width(uint64_t{g.errors} * 1000000 / n);
                    out_groups.push_back(g);
                    count[id] = errors[id] = 0;
                }
                bytes[h + 1] = text;
            }
        });
        // Records are no longer needed: free them before the output's pages count.
        threads.run(workers, [&](size_t w) {
            for (auto [p, n] : part[w].pools) munmap(p, n);
        });
        for (uint32_t h = 0; h < HOURS; ++h) bytes[h + 1] += bytes[h];
        if (ftruncate(out, static_cast<off_t>(bytes[HOURS]))) throw std::runtime_error("ftruncate failed");
        char* text = static_cast<char*>(mmap(nullptr, bytes[HOURS], PROT_WRITE, MAP_SHARED, out, 0));
        if (text == MAP_FAILED) throw std::runtime_error("mmap of output failed");
        next_hour = 0;
        threads.run(workers, [&](size_t) {
            for (uint32_t h; (h = next_hour.fetch_add(1)) < HOURS;) {
                uint32_t day = (h + 18) / 24, hh = (h + 18) % 24;
                char stamp[22] = "2026-10-30T00:00:00Z ";
                if (day >= 2) stamp[6] = '1', stamp[8] = '0';
                stamp[9] = static_cast<char>('0' + (day < 2 ? day : day - 1));
                stamp[11] = static_cast<char>('0' + hh / 10);
                stamp[12] = static_cast<char>('0' + hh % 10);
                char* o = text + bytes[h];
                for (const Group& g : groups[h]) {
                    memcpy(o, stamp, 21);
                    o += 21;
                    memcpy(o, names->full[g.id], names->lens[g.id]);
                    o += names->lens[g.id];
                    for (uint64_t x : {uint64_t{g.n}, uint64_t{g.p50}, uint64_t{g.p95}, uint64_t{g.p99}, uint64_t{g.errors}})
                        *o++ = ' ', o = put(o, x);
                    *o++ = ' ';
                    o = put(o, uint64_t{g.errors} * 1000000 / g.n);
                    *o++ = '\n';
                }
                std::vector<Group>().swap(groups[h]);
            }
        });
        munmap(text, bytes[HOURS]);
        return close(out) ? 1 : 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
