#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <fcntl.h>
#include <immintrin.h>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Parallel exact CSV join. Piece boundaries are found from the spec's record
// start rule. Names are compared by their raw quoted bytes (unescaping
// is injective), interned once in a shared lock-free table. Country bits beside each name
// credit exactly one worker-local distinct count on their atomic first set.
constexpr uint32_t SLOTS = 1u << 18, NONE = UINT16_MAX;

static const char* map_file(const std::string& path, size_t& size) {
    int fd = open(path.c_str(), O_RDONLY);
    struct stat st{};
    if (fd < 0 || fstat(fd, &st) != 0) throw std::runtime_error("cannot open " + path);
    size = static_cast<size_t>(st.st_size);
    // Vector loads may read past the end: back the mapping with zero pages.
    void* base = mmap(nullptr, size + 8192, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED || (size && mmap(base, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED))
        throw std::runtime_error("mmap failed");
    close(fd);
    return static_cast<const char*>(base);
}
static uint32_t quotes(const char* p) {
    return static_cast<uint32_t>(_mm256_movemask_epi8(
        _mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)), _mm256_set1_epi8('"'))));
}

// Parses the decimal digits at p (up to 8 at once); p ends on the first non-digit.
[[gnu::always_inline]] static inline uint64_t digits(const char*& p) {
    uint64_t v;
    memcpy(&v, p, 8);
    uint64_t x = v ^ 0x3030303030303030ull, stop = ((x + 0x7676767676767676ull) | x) & 0x8080808080808080ull;
    if (!stop) {
        uint64_t n = 0;
        while (static_cast<unsigned char>(*p - '0') < 10) n = n * 10 + static_cast<uint64_t>(*p++ - '0');
        return n;
    }
    uint32_t len = static_cast<uint32_t>(__builtin_ctzll(stop)) >> 3;
    p += len;
    if (!len) return 0;
    x = (x << (64 - 8 * len)) & 0x0F0F0F0F0F0F0F0Full;
    x = (x * 10 + (x >> 8)) & 0x00FF00FF00FF00FFull;
    x = (x * 100 + (x >> 16)) & 0x0000FFFF0000FFFFull;
    return (x * 10000 + (x >> 32)) & 0xFFFFFFFFull;
}

struct alignas(64) Group {
    uint64_t count = 0, ids = 0, name_bytes = 0, note_bytes = 0;
    int64_t sum = 0, min = INT64_MAX, max = INT64_MIN;
    uint64_t distinct = 0;
};

// Field end for a field starting at p: the closing quote's successor for a
// quoted field (pairs counted in `pairs`), else the next comma or terminator.
[[gnu::always_inline]] static inline const char* field(const char* p, const char*& begin, const char*& end, uint32_t& pairs) {
    pairs = 0;
    if (*p != '"') {
        begin = p;
        while (*p != ',' && *p != '\n' && *p != '\r') ++p;
        end = p;
        return p;
    }
    begin = ++p;
    for (;;) {
        uint32_t m = quotes(p);
        if (!m) { p += 32; continue; }
        p += __builtin_ctz(m);
        if (p[1] != '"') break;
        p += 2, ++pairs;
    }
    end = p;
    return p + 1;
}

// Zaps this range's page-table entries now, in parallel, instead of leaving
// the whole mapping to a single-threaded teardown at exit.
static void drop(const char* a, const char* b) {
    uintptr_t s = (reinterpret_cast<uintptr_t>(a) + 4095) & ~uintptr_t{4095}, e = reinterpret_cast<uintptr_t>(b) & ~uintptr_t{4095};
    if (s < e) madvise(reinterpret_cast<void*>(s), e - s, MADV_DONTNEED);
}

int main(int argc, char** argv) {
    // The scoring box backs anonymous memory with 2 MiB pages by default, which
    // charges whole huge pages for partly used regions: use 4 KiB pages.
    prctl(PR_SET_THP_DISABLE, 1, 0, 0, 0);
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
        size_t rsize, size;
        const char* regions = map_file(data_dir + "/regions.csv", rsize);
        const char* data = map_file(data_dir + "/records.csv", size);

        // region_id -> group: country codes in byte order, then _UNMATCHED.
        constexpr uint8_t MISSING = 255;  // the frozen lookup has 240 countries
        std::vector<uint8_t> region;
        std::vector<uint16_t> group(26 * 26 * 26, NONE);
        if (rsize > 1) {  // ids ascend, so the last line holds the largest
            const char* last = regions + rsize - 1;
            while (last > regions && last[-1] != '\n') --last;
            if (static_cast<unsigned char>(*last - '0') < 10) region.assign(digits(last) + 1, MISSING);
        }
        std::vector<uint32_t> codes;
        for (const char *p = static_cast<const char*>(memchr(regions, '\n', rsize)) + 1, *end = regions + rsize; p < end;) {
            uint64_t id = digits(p);
            if (id >= region.size()) region.resize(std::max<size_t>(id + 1, region.size() * 2), MISSING);
            uint32_t code = static_cast<uint32_t>(((p[1] - 'A') * 26 + p[2] - 'A') * 26 + p[3] - 'A');
            uint16_t& k = group[code];
            if (k == NONE) k = static_cast<uint16_t>(codes.size()), codes.push_back(code);
            if (k >= MISSING) throw std::runtime_error("too many countries");
            region[id] = static_cast<uint8_t>(k);
            p += 5;
        }
        const uint32_t G = static_cast<uint32_t>(codes.size()) + 1;
        std::vector<uint32_t> order(G);  // groups in output order: codes ascending, _UNMATCHED last
        for (uint32_t k = 0; k < G; ++k) order[k] = k;
        std::sort(order.begin(), order.end() - 1, [&](uint32_t a, uint32_t b) { return codes[a] < codes[b]; });

        // Exact chunk starts: a byte is inside quotes iff an odd number of
        // quotes precede it. Each chunk begins after its first outside LF.
        const size_t pieces = workers * 8;  // claimed in turn, for an even finish
        std::vector<size_t> cut(pieces + 1, size);
        std::vector<std::thread> list;
        auto run = [&](auto f) {
            for (size_t i = 1; i < workers; ++i) list.emplace_back(f, i);
            f(0);
            for (auto& t : list) t.join();
            list.clear();
        };
        // Piece starts: by spec 2.2 an LF inside a note is followed by a
        // lowercase word or a quote and names never hold an LF, so an LF
        // followed by a digit (after an optional byte order mark) ends a record.
        cut[0] = static_cast<size_t>(static_cast<const char*>(memchr(data, '\n', size)) - data) + 1;  // header line
        for (size_t c = 1; c < pieces; ++c) {
            size_t p = std::max(cut[c - 1], size * c / pieces);
            auto digit = [&](size_t i) { return i < size && static_cast<unsigned char>(data[i] - '0') < 10; };
            while (p < size && !(data[p - 1] == '\n' && (digit(p) || (!memcmp(data + p, "\xEF\xBB\xBF", 3) && digit(p + 3))))) ++p;
            cut[c] = p;
        }

        // Shared name table. A slot holds a 16-bit hash tag and a pointer to an
        // arena record {length, country bits, bytes}: the inserting worker writes the
        // record, then publishes it with a release CAS; readers acquire it.
        auto* slots = static_cast<uint64_t*>(calloc(SLOTS, sizeof(uint64_t)));
        std::atomic<size_t> claim{0};
        std::vector<std::vector<std::unique_ptr<char[]>>> arenas(workers);  // live until exit
        std::vector<std::vector<Group>> part(workers, std::vector<Group>(G));
        // Country membership lives beside each name, eliminating an independent
        // random bitset lookup. Atomic first-set detection credits exactly one
        // worker-local distinct counter for each (country, exact name) pair.
        const size_t bitwords = (G + 63) / 64, header = 8 + bitwords * 8;
        // Branch-free hash of the first 16 and last 8 bytes and the length;
        // the full byte comparison in intern() keeps equality exact.
        auto hash = [](const char* s, uint32_t len) {
            uint64_t a, b, c;
            memcpy(&a, s, 8), memcpy(&b, s + 8, 8), memcpy(&c, s + len - 8, 8);  // all inside the mapping
            a = _bzhi_u64(a, 8 * len), b = len > 8 ? _bzhi_u64(b, 8 * (len - 8)) : 0, c = len > 16 ? c : 0;
            uint64_t h = (a * 0x9E3779B97F4A7C15ull ^ b * 0xC2B2AE3D27D4EB4Full ^ c * 0x165667B19E3779F9ull ^ len) * 0xD6E8FEB86659FD93ull;
            return h ^ (h >> 32);
        };
        auto intern = [&](const char* s, uint32_t len, uint64_t h, std::vector<std::unique_ptr<char[]>>& arena, size_t& room) -> char* {
            const uint64_t tag = h >> 48 << 48;
            char* rec = nullptr;
            for (uint32_t i = static_cast<uint32_t>(h) & (SLOTS - 1);; i = (i + 1) & (SLOTS - 1)) {
                uint64_t e = std::atomic_ref<uint64_t>(slots[i]).load(std::memory_order_acquire);
                if (!e) {
                    if (!rec) {
                        size_t need = (header + len + 7) & ~size_t{7};
                        if (room < need) arena.emplace_back(new char[std::max<size_t>(need, 1 << 16)]), room = std::max<size_t>(need, 1 << 16);
                        rec = arena.back().get() + (std::max<size_t>(need, 1 << 16) - room);
                        room -= need;
                        memcpy(rec, &len, 4), memcpy(rec + header, s, len);
                        auto* bits = reinterpret_cast<std::atomic<uint64_t>*>(rec + 8);
                        for (size_t k = 0; k < bitwords; ++k) new (bits + k) std::atomic<uint64_t>{0};
                    }
                    if (std::atomic_ref<uint64_t>(slots[i]).compare_exchange_strong(e, tag | reinterpret_cast<uint64_t>(rec), std::memory_order_acq_rel)) return rec;
                }
                const char* other = reinterpret_cast<const char*>(e & ((1ull << 48) - 1));
                uint32_t n;
                memcpy(&n, other, 4);
                if ((e >> 48 << 48) == tag && n == len && !memcmp(other + header, s, len)) return const_cast<char*>(other);
            }
        };
        run([&](size_t w) {
            auto& g = part[w];
            auto& arena = arenas[w];
            size_t room = 0;
            // Names resolve eight records late: their slots and records are prefetched.
            struct Pending { const char* s; uint32_t len, k; uint64_t h; } ring[8];
            uint32_t pending = 0;
            auto resolve = [&](const Pending& q) {
                char* rec = intern(q.s, q.len, q.h, arena, room);
                auto* bits = reinterpret_cast<std::atomic<uint64_t>*>(rec + 8);
                uint64_t mask = 1ull << (q.k % 64);
                // Membership bits only grow. A previously set bit needs no
                // write ownership; fetch_or still arbitrates every first set.
                auto& word = bits[q.k / 64];
                if (!(word.load(std::memory_order_relaxed) & mask)) {
                    uint64_t old = word.fetch_or(mask, std::memory_order_relaxed);
                    g[q.k].distinct += (old & mask) == 0;
                }
            };
            for (size_t c; (c = claim.fetch_add(1)) < pieces;)
            for (const char *p = data + cut[c], *end = data + cut[c + 1]; p < end;) {
                __builtin_prefetch(p + 2048);  // the hardware prefetcher stops at page ends
                if (static_cast<unsigned char>(p[0]) == 0xEF && static_cast<unsigned char>(p[1]) == 0xBB &&
                    static_cast<unsigned char>(p[2]) == 0xBF)
                    p += 3;
                uint64_t id = digits(p);
                const char *nb, *ne, *tb, *te;
                uint32_t npairs, tpairs;
                p = field(p + 1, nb, ne, npairs);
                p = field(p + 1, tb, te, tpairs);
                uint64_t r = digits(++p);
                bool negative = *++p == '-';
                p += negative;
                int64_t cents = static_cast<int64_t>(digits(p)) * 100 + (p[1] - '0') * 10 + (p[2] - '0');
                p += 3;
                if (negative) cents = -cents;
                p += *p == '\r';
                p += p < end && *p == '\n';
                Group& x = g[r < region.size() && region[r] != MISSING ? region[r] : G - 1];
                ++x.count;
                x.ids += id;
                x.sum += cents;
                x.min = std::min(x.min, cents);
                x.max = std::max(x.max, cents);
                x.name_bytes += static_cast<uint64_t>(ne - nb) - npairs;
                x.note_bytes += static_cast<uint64_t>(te - tb) - tpairs;
                uint32_t len = static_cast<uint32_t>(ne - nb);
                uint64_t h = hash(nb, len);
                __builtin_prefetch(&slots[h & (SLOTS - 1)]);
                Pending& half = ring[(pending + 4) % 8];
                if (pending >= 4) __builtin_prefetch(reinterpret_cast<const char*>(std::atomic_ref<uint64_t>(slots[half.h & (SLOTS - 1)]).load(std::memory_order_relaxed) & ((1ull << 48) - 1)));
                Pending& slot = ring[pending % 8];
                if (pending >= 8) resolve(slot);
                slot = {nb, len, static_cast<uint32_t>(&x - g.data()), h};
                ++pending;
            }
            for (uint32_t i = pending > 8 ? pending - 8 : 0; i < pending; ++i) resolve(ring[i % 8]);
        });
        run([&](size_t w) { drop(data + size * w / workers, data + size * (w + 1) / workers); });  // names live in the arenas
        std::string out;
        char line[256];
        auto money = [](char* o, int64_t c) {
            uint64_t a = c < 0 ? 0 - static_cast<uint64_t>(c) : static_cast<uint64_t>(c);
            return sprintf(o, "%s%llu.%02llu", c < 0 ? "-" : "", static_cast<unsigned long long>(a / 100),
                           static_cast<unsigned long long>(a % 100));
        };
        for (uint32_t k : order) {
            Group t;
            for (auto& g : part) {
                const Group& x = g[k];
                t.count += x.count, t.ids += x.ids, t.sum += x.sum, t.name_bytes += x.name_bytes, t.note_bytes += x.note_bytes;
                t.min = std::min(t.min, x.min), t.max = std::max(t.max, x.max);
                t.distinct += x.distinct;
            }
            if (!t.count) continue;
            char* o = line;
            if (k == G - 1) o += sprintf(o, "_UNMATCHED");
            else o += sprintf(o, "%c%c%c", 'A' + codes[k] / 676, 'A' + codes[k] / 26 % 26, 'A' + codes[k] % 26);
            o += sprintf(o, " %llu %llu ", static_cast<unsigned long long>(t.count), static_cast<unsigned long long>(t.ids));
            o += money(o, t.sum);
            *o++ = ' ';
            o += money(o, t.min);
            *o++ = ' ';
            o += money(o, t.max);
            o += sprintf(o, " %llu %llu %llu\n", static_cast<unsigned long long>(t.distinct),
                         static_cast<unsigned long long>(t.name_bytes), static_cast<unsigned long long>(t.note_bytes));
            out.append(line, static_cast<size_t>(o - line));
        }
        FILE* f = fopen((out_dir + "/aggregates.txt").c_str(), "wb");
        if (!f || fwrite(out.data(), 1, out.size(), f) != out.size() || fclose(f)) throw std::runtime_error("write failed");
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
