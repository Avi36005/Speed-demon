#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <fcntl.h>
#include <mutex>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Two exact passes over the mapped log. Pass one records which hours start a
// line in each 16 KiB block. Pass two answers each hour independently, reading
// only that hour's blocks, so only one hour's latencies are held per worker.
constexpr uint32_t HOURS = 72, SLOTS = 8192, CAP = 4096, KEY = 64, BLOCK = 14;
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

struct Table {
    std::vector<uint16_t> table = std::vector<uint16_t>(SLOTS);  // id + 1, 0 is empty
    char* keys = static_cast<char*>(calloc(size_t{CAP}, KEY));  // only used pages are backed
    std::vector<uint32_t> lens = std::vector<uint32_t>(CAP);
    std::vector<std::string_view> names;

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
                if (id >= CAP) throw std::runtime_error("unsupported endpoint set");
                table[i] = static_cast<uint16_t>(id + 1);
                memcpy(&keys[size_t{id} * KEY], s, std::min(len, KEY));
                lens[id] = len;
                names.emplace_back(s, len);
                return id;
            }
            const char* k = &keys[size_t{--id} * KEY];
            if (lens[id] == len && !(~eq64(v, load32(k), load32(k + 32)) & need) && (len <= KEY || !memcmp(names[id].data() + KEY, s + KEY, len - KEY)))
                return id;
        }
    }
};

// The frozen grammar fixes UTC timestamps and this 72-hour span.
static uint32_t hour_of(const char* p) {
    uint32_t day = (p[8] - '0') * 10 + p[9] - '0' - 30 + (p[6] - '0') * 31;
    return day * 24 + (p[11] - '0') * 10 + p[12] - '0' - 18;
}
// Lines are at least 40 bytes long.
static const char* next_line(const char* p) {
    for (p += 39;; p += 32)
        if (uint32_t m = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(load32(p), _mm256_set1_epi8('\n')))))
            return p + __builtin_ctz(m) + 1;
}

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
template <class T>
static void select(T* first, T* last, T* k) {
    while (last - first > 24) {
        T *mid = first + (last - first) / 2, *back = last - 1;
        if (*mid < *first) std::swap(*mid, *first);
        if (*back < *mid) std::swap(*back, *mid);
        if (*mid < *first) std::swap(*mid, *first);
        std::swap(*mid, *back);
        T pivot = *back, *j = first;
        for (T* i = first; i < back; ++i) {
            T x = *i;
            *i = *j;
            *j = x;
            j += x < pivot;
        }
        std::swap(*j, *back);
        if (k == j) return;
        if (k < j) last = j;
        else first = j + 1;
    }
    for (T* i = first + 1; i < last; ++i) {
        T x = *i, *j = i;
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
        int out = open((out_dir + "/summary.txt").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out < 0) throw std::runtime_error("cannot create summary.txt");
        if (!size) return close(out);
        // Vector loads may read past the last line: back the mapping with
        // extra anonymous zero pages.
        void* base = mmap(nullptr, size + 8192, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED || mmap(base, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED)
            throw std::runtime_error("mmap failed");
        const char* data = static_cast<const char*>(base);
        // Worker ranges start at block boundaries, so block summaries are private.
        const size_t blocks = (size >> BLOCK) + 1;
        std::vector<uint64_t> mask(2 * blocks);
        std::vector<uint32_t> first(blocks, UINT32_MAX);
        std::vector<size_t> cut(workers + 1, size);
        for (size_t w = 0; w < workers; ++w) {
            size_t a = (size * w / workers) >> BLOCK << BLOCK;
            while (a > 0 && a < size && data[a - 1] != '\n') ++a;
            cut[w] = std::max(a, w ? cut[w - 1] : 0);
        }
        Threads threads;
        threads.run(workers, [&](size_t w) {
            for (const char* p = data + cut[w]; p < data + cut[w + 1]; p = next_line(p)) {
                size_t b = static_cast<size_t>(p - data) >> BLOCK;
                uint32_t h = hour_of(p);
                mask[2 * b + h / 64] |= 1ull << (h % 64);
                if (first[b] == UINT32_MAX) first[b] = static_cast<uint32_t>(p - data);
            }
        });
        // Hours are claimed in order, answered privately and written in order.
        std::atomic<uint32_t> next{0};
        std::mutex lock;
        std::vector<std::string> ready(HOURS);
        std::vector<bool> done(HOURS);
        uint32_t written = 0;
        threads.run(workers, [&](size_t) {
            Table table;
            std::vector<uint32_t> count(CAP), errors(CAP), start(CAP), fill(CAP), ids;
            std::vector<uint64_t> rows;  // id << 32 | latency
            const __m256i space = _mm256_set1_epi8(' ');
            const __m128i newline = _mm_set1_epi8('\n');
            for (uint32_t h; (h = next.fetch_add(1)) < HOURS;) {
                rows.clear();
                for (size_t b = 0; b < blocks; ++b) {
                    if (!(mask[2 * b + h / 64] >> (h % 64) & 1)) continue;
                    const char* stop = data + std::min(size, (b + 1) << BLOCK);
                    for (const char* p = data + first[b]; p < stop;) {
                        if (hour_of(p) != h) {
                            p = next_line(p);
                            continue;
                        }
                        uint64_t w = load8(p + 31) ^ 0x2020202020202020ull;  // methods are 3 to 7 bytes
                        const char* s = p + 32 + (__builtin_ctzll((w - 0x0101010101010101ull) & ~w & 0x8080808080808080ull) >> 3);
                        __m256i v[2] = {load32(s), load32(s + 32)};
                        uint64_t m = eq64(v, space, space);
                        uint32_t len = m ? static_cast<uint32_t>(__builtin_ctzll(m)) : 64;
                        while (s[len] != ' ') ++len;
                        uint32_t id = table.lookup(s, len, v);
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
                        if (!count[id]++) ids.push_back(id);
                        errors[id] += e[1] == '5';
                        rows.push_back(uint64_t{id} << 32 | latency);
                        p = e + 6 + digits;
                    }
                }
                // Group this hour's latencies by endpoint in exact byte order.
                std::sort(ids.begin(), ids.end(), [&](uint32_t a, uint32_t b) { return table.names[a] < table.names[b]; });
                // In-place counting sort: each swap settles one row in its group.
                uint32_t at = 0;
                for (uint32_t id : ids) start[id] = at, at += count[id], fill[id] = start[id];
                for (uint32_t id : ids)
                    for (uint32_t end = start[id] + count[id]; fill[id] < end;) {
                        uint32_t t = static_cast<uint32_t>(rows[fill[id]] >> 32);
                        if (t == id) ++fill[id];
                        else std::swap(rows[fill[id]], rows[fill[t]++]);
                    }
                std::string buf(1024, '\0');
                size_t used = 0;
                uint32_t day = (h + 18) / 24, hh = (h + 18) % 24;
                char stamp[22] = "2026-10-30T00:00:00Z ";
                if (day >= 2) stamp[6] = '1', stamp[8] = '0';
                stamp[9] = static_cast<char>('0' + (day < 2 ? day : day - 1));
                stamp[11] = static_cast<char>('0' + hh / 10);
                stamp[12] = static_cast<char>('0' + hh % 10);
                for (uint32_t id : ids) {
                    uint64_t n = count[id];
                    if (buf.size() - used < 300) buf.resize(buf.size() * 2);
                    uint64_t* first_value = rows.data() + start[id];
                    uint64_t* last = first_value + n;
                    char* o = buf.data() + used;
                    memcpy(o, stamp, 21);
                    o += 21;
                    memcpy(o, table.names[id].data(), table.names[id].size());
                    o += table.names[id].size();
                    *o++ = ' ';
                    o = put(o, n);
                    uint64_t* rest = first_value;  // one group: rows order by latency
                    for (uint64_t pct : {50, 95, 99}) {
                        uint64_t* rank = first_value + (pct * n + 99) / 100 - 1;
                        if (rank >= rest) {
                            select(rest, last, rank);
                            rest = rank + 1;
                        }
                        *o++ = ' ';
                        o = put(o, static_cast<uint32_t>(*rank));
                    }
                    *o++ = ' ';
                    o = put(o, errors[id]);
                    *o++ = ' ';
                    o = put(o, errors[id] * uint64_t{1000000} / n);
                    *o++ = '\n';
                    used = static_cast<size_t>(o - buf.data());
                    count[id] = errors[id] = 0;
                }
                ids.clear();
                buf.resize(used);
                // Whoever finishes an hour writes every consecutive finished hour.
                std::lock_guard<std::mutex> guard(lock);
                ready[h] = std::move(buf);
                done[h] = true;
                for (; written < HOURS && done[written]; ++written) {
                    const std::string& text = ready[written];
                    for (size_t off = 0; off < text.size();) {
                        ssize_t k = write(out, text.data() + off, text.size() - off);
                        if (k <= 0) throw std::runtime_error("write failed");
                        off += static_cast<size_t>(k);
                    }
                    std::string().swap(ready[written]);
                }
            }
        });
        return close(out) ? 1 : 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
