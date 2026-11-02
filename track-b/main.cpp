#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <immintrin.h>
#include <sched.h>
#include <stdexcept>
#include <span>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Exact query-vocabulary index. Workers tokenize disjoint 64-aligned document
// ranges once. Terms a sample predicts to be common get bitmaps; every other
// term keeps compact sorted 16-bit runs per 8192-document block.
constexpr uint32_t NONE = UINT32_MAX;

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

static __m256i in_range(__m256i v, char lo, char hi) {
    return _mm256_and_si256(_mm256_cmpgt_epi8(v, _mm256_set1_epi8(static_cast<char>(lo - 1))),
                            _mm256_cmpgt_epi8(_mm256_set1_epi8(static_cast<char>(hi + 1)), v));
}
// Token bytes of 32 input bytes, and the same bytes ASCII-lowercased.
static uint32_t classify(const char* p, __m256i& lower) {
    __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
    lower = _mm256_or_si256(v, _mm256_and_si256(in_range(v, 'A', 'Z'), _mm256_set1_epi8(32)));
    return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(in_range(lower, 'a', 'z'), in_range(lower, '0', '9'))));
}
alignas(32) static const char ONES[64] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static __m256i load32(const char* p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)); }
static uint64_t load8(const char* p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}
// A key: lowered token bytes, zero-padded to 32. Longer tokens are never terms.
static __m256i key_of(const char* p, uint32_t len) { return _mm256_and_si256(load32(p), load32(ONES + 32 - len)); }
static bool same(__m256i a, __m256i b) { return _mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b)) == -1; }
static bool alnum(char c) { return static_cast<unsigned char>((c | 32) - 'a') < 26 || static_cast<unsigned char>(c - '0') < 10; }
static char lower(char c) { return static_cast<unsigned char>(c - 'A') < 26 ? static_cast<char>(c + 32) : c; }

// Lowers a document into a scratch buffer with one token-byte bit per byte,
// then calls f(lowered bytes, length) for every maximal token.
struct Scratch {
    std::vector<char> buf;
    std::vector<uint64_t> masks;
};
template <class F>
static void tokens(const char* t, const char* end, Scratch& sc, F f) {
    size_t n = static_cast<size_t>(end - t), blocks = (n + 63) / 64;
    if (sc.buf.size() < blocks * 64 + 128) sc.buf.resize(blocks * 128 + 128), sc.masks.resize(blocks * 2 + 1);
    char* buf = sc.buf.data() + 64;  // front padding for branch-free hashing
    for (size_t b = 0; b < blocks; ++b) {
        __m256i lo, hi;
        uint64_t m = classify(t + 64 * b, lo) | uint64_t{classify(t + 64 * b + 32, hi)} << 32;
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(buf + 64 * b), lo);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(buf + 64 * b + 32), hi);
        sc.masks[b] = m;
    }
    if (n % 64) sc.masks[blocks - 1] &= (1ull << (n % 64)) - 1;
    sc.masks[blocks] = 0;
    uint64_t carry = 0;
    for (size_t b = 0; b < blocks; ++b) {
        uint64_t m = sc.masks[b];
        for (uint64_t s = m & ~(m << 1 | carry); s; s &= s - 1) {
            uint32_t i = static_cast<uint32_t>(__builtin_ctzll(s));
            uint32_t len = static_cast<uint32_t>(__builtin_ctzll(~(m >> i)));
            if (i + len == 64)
                for (size_t c = b + 1;; ++c) {
                    uint64_t x = ~sc.masks[c];
                    uint32_t k = x ? static_cast<uint32_t>(__builtin_ctzll(x)) : 64;
                    len += k;
                    if (k < 64) break;
                }
            f(buf + 64 * b + i, len);
        }
        carry = m >> 63;
    }
}

// Open addressing over 2^18 slots: a 14-bit fingerprint and an 18-bit id.
// Fingerprints only filter probes; a full 32-byte comparison decides.
struct Vocabulary {
    static constexpr uint32_t BITS = 18, MASK = (1u << BITS) - 1;
    std::vector<uint32_t> slots = std::vector<uint32_t>(1u << BITS, NONE);
    std::vector<__m256i> keys;
    std::vector<uint32_t> lens;
    static uint64_t hash(const char* p, uint32_t len) {
        // Branch-free: callers keep 8 readable bytes before p.
        uint64_t lo = _bzhi_u64(load8(p), 8 * len), hi = load8(p + len - 8) & (0 - uint64_t{len > 8});
        return ((lo * 0x9E3779B97F4A7C15ull) ^ (hi * 0xC2B2AE3D27D4EB4Full) ^ len) * 0xD6E8FEB86659FD93ull;
    }
    uint32_t find(const char* p, uint32_t len) const { return len > 32 ? NONE : find(p, len, hash(p, len)); }
    const uint32_t* slot(uint64_t h) const { return &slots[h >> (64 - BITS)]; }
    uint32_t find(const char* p, uint32_t len, uint64_t h) const {
        uint32_t fp = static_cast<uint32_t>(h >> 18) & ~MASK;
        for (uint32_t i = static_cast<uint32_t>(h >> (64 - BITS));; i = (i + 1) & MASK) {
            uint32_t e = slots[i];
            if (e == NONE) return NONE;
            if ((e & ~MASK) == fp && same(keys[e & MASK], key_of(p, len))) return e & MASK;
        }
    }
    // s may be raw text: hashing and keys mask every byte past len.
    uint32_t intern(const char* s, uint32_t len, uint64_t h) {
        if (uint32_t id = find(s, len, h); id != NONE) return id;
        if (keys.size() >= MASK / 2) throw std::runtime_error("vocabulary too large");
        uint32_t i = static_cast<uint32_t>(h >> (64 - BITS));
        while (slots[i] != NONE) i = (i + 1) & MASK;
        keys.push_back(key_of(s, len));
        lens.push_back(len);
        slots[i] = (static_cast<uint32_t>(h >> 18) & ~MASK) | static_cast<uint32_t>(keys.size() - 1);
        return static_cast<uint32_t>(keys.size() - 1);
    }
};

// Exact phrase test around each occurrence of the phrase's rarest term:
// neighbouring tokens are read directly from the raw text in both directions.
static bool phrase_in(const char* t, size_t n, const std::vector<const char*>& key, const std::vector<uint32_t>& len, size_t a) {
    auto equal = [&](size_t s, size_t e, size_t j) {
        if (e - s != len[j]) return false;
        for (size_t i = 0; i < len[j]; ++i) if (lower(t[s + i]) != key[j][i]) return false;
        return true;
    };
    const char* k = key[a];
    const __m256i c0 = _mm256_set1_epi8(k[0]), c1 = _mm256_set1_epi8(len[a] > 1 ? k[1] : 0), up = _mm256_set1_epi8(32);
    for (size_t base = 0; base < n; base += 32) {
        uint32_t m = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_or_si256(load32(t + base), up), c0)));
        if (len[a] > 1) m &= static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_or_si256(load32(t + base + 1), up), c1)));
        for (; m; m &= m - 1) {
            size_t i = base + static_cast<size_t>(__builtin_ctz(m));
            if (i + len[a] > n || (i && alnum(t[i - 1])) || (i + len[a] < n && alnum(t[i + len[a]])) || !equal(i, i + len[a], a))
                continue;
            bool ok = true;
            for (size_t j = a, pos = i; ok && j-- > 0;) {
                size_t e = pos;
                while (e && !alnum(t[e - 1])) --e;
                size_t s = e;
                while (s && alnum(t[s - 1])) --s;
                ok = s < e && equal(s, e, j);
                pos = s;
            }
            for (size_t j = a + 1, pos = i + len[a]; ok && j < key.size(); ++j) {
                size_t s = pos;
                while (s < n && !alnum(t[s])) ++s;
                size_t e = s;
                while (e < n && alnum(t[e])) ++e;
                ok = s < e && equal(s, e, j);
                pos = e;
            }
            if (ok) return true;
        }
    }
    return false;
}

// A query's terms (NONE when no vocabulary holds them) and operators live in
// flat arrays: term i of query q is all_terms[q.at + i], its operator all_ops[q.at + i].
struct Query {
    bool phrase;
    uint32_t at, n;
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
        size_t qsize, dsize;
        const char* qtext = map_file(data_dir + "/queries.txt", qsize);
        const char* docs = map_file(data_dir + "/docs.txt", dsize);

        Vocabulary vocab;
        vocab.keys.reserve(qsize / 8), vocab.lens.reserve(qsize / 8);
        std::vector<Query> queries;
        queries.reserve(qsize / 16);
        struct Raw {
            const char* s;
            uint32_t len;
            uint64_t h;
        };
        std::vector<Raw> raw;
        std::vector<uint32_t> all_terms;
        std::vector<char> all_ops;
        all_terms.reserve(qsize / 4), all_ops.reserve(qsize / 4);
        raw.reserve(qsize / 4);
        for (const char *p = qtext, *end = qtext + qsize; p < end;) {
            const char* nl = static_cast<const char*>(memchr(p, '\n', static_cast<size_t>(end - p)));
            if (!nl) nl = end;
            Query q{p[0] == 'P', static_cast<uint32_t>(all_terms.size()), 0};
            const char* s = p + (q.phrase ? 8 : 5);
            const char* e = q.phrase ? nl - 1 : nl;
            for (bool term = true; s < e; term = q.phrase || !term) {
                const char* w = s;
                while (w < e && *w != ' ') ++w;
                if (!term) all_ops.back() = s[0];
                else if (++q.n, all_ops.push_back(0), w - s > 32) all_terms.push_back(NONE);
                else {
                    // Hash now and prefetch; intern after the whole file is read.
                    uint64_t h = Vocabulary::hash(s, static_cast<uint32_t>(w - s));
                    __builtin_prefetch(vocab.slot(h));
                    all_terms.push_back(static_cast<uint32_t>(raw.size()));
                    raw.push_back({s, static_cast<uint32_t>(w - s), h});
                }
                s = w + 1;
            }
            queries.push_back(q);
            p = nl + 1;
        }
        std::vector<uint32_t> ids(raw.size());
        for (size_t i = 0; i < raw.size(); ++i) ids[i] = vocab.intern(raw[i].s, raw[i].len, raw[i].h);
        for (uint32_t& t : all_terms) if (t != NONE) t = ids[t];
        const size_t V = vocab.keys.size();

        // Document ranges start at multiples of 64 so bitmap words are private.
        std::vector<const char*> cut(workers + 1, docs + dsize);
        std::vector<uint32_t> first(workers + 1, 0);
        cut[0] = docs;
        for (size_t i = 1; i < workers; ++i) {
            const char* p = std::max(cut[i - 1], docs + dsize * i / workers);
            while (p > docs && p < docs + dsize && p[-1] != '\n') ++p;
            for (; p < docs + dsize; p = static_cast<const char*>(memchr(p, '\n', docs + dsize - p)) + 1) {
                uint32_t id = 0;
                for (const char* d = p; *d != '\t'; ++d) id = id * 10 + static_cast<uint32_t>(*d - '0');
                if (id % 64 == 0) { first[i] = id; break; }
            }
            cut[i] = p;
            if (p == docs + dsize) first[i] = NONE;
        }
        // N is the id of the final line plus one.
        uint32_t N = 0;
        if (dsize) {
            const char* p = docs + dsize - 1;
            while (p > docs && p[-1] != '\n') --p;
            for (; *p != '\t'; ++p) N = N * 10 + static_cast<uint32_t>(*p - '0');
            ++N;
        }
        const size_t words = (N + 63) / 64;
        Threads threads;
        auto scan = [&](size_t w, size_t limit, auto&& hit) {
            std::vector<uint32_t> last(V, NONE);
            Scratch sc;
            struct Token {
                const char* key;
                uint32_t len;
                uint64_t h;
            };
            std::vector<Token> pending;
            uint32_t doc = first[w] == NONE ? 0 : first[w];
            for (const char* p = cut[w]; p < cut[w + 1] && limit; ++doc, --limit) {
                const char* t = static_cast<const char*>(memchr(p, '\t', 12)) + 1;
                const char* nl = static_cast<const char*>(memchr(t, '\n', static_cast<size_t>(cut[w + 1] - t)));
                hit(doc, p, NONE);
                // Hash every token first and prefetch its slot, then resolve.
                pending.clear();
                tokens(t, nl, sc, [&](const char* key, uint32_t len) {
                    if (len > 32) return;
                    uint64_t h = Vocabulary::hash(key, len);
                    __builtin_prefetch(vocab.slot(h));
                    pending.push_back({key, len, h});
                });
                for (const Token& k : pending) {
                    uint32_t id = vocab.find(k.key, k.len, k.h);
                    if (id != NONE && last[id] != doc) last[id] = doc, hit(doc, p, id);
                }
                p = nl + 1;
            }
        };
        // A small sample predicts the dense terms; their hits go straight into
        // bitmaps. The prediction only picks a representation, never an answer.
        std::vector<std::vector<uint32_t>> counts(workers, std::vector<uint32_t>(V));
        std::vector<uint32_t> sampled(workers);
        const size_t per = std::max<size_t>(64, 8192 / workers);
        threads.run(workers, [&](size_t w) {
            scan(w, per, [&](uint32_t, const char*, uint32_t id) { id == NONE ? ++sampled[w] : ++counts[w][id]; });
        });
        std::vector<uint32_t> bitmap_of(V, NONE), df(V);
        size_t nbitmaps = 0, total = 0;
        for (size_t w = 0; w < workers; ++w) total += sampled[w];
        for (size_t t = 0; t < V; ++t) {
            size_t n = 0;
            for (size_t w = 0; w < workers; ++w) n += counts[w][t], counts[w][t] = 0;
            if (n * 40 > total) bitmap_of[t] = static_cast<uint32_t>(nbitmaps++);
        }
        auto* bitmaps = static_cast<uint64_t*>(calloc(nbitmaps * words + 1, 8));
        auto* offsets = static_cast<uint32_t*>(calloc(N + 1, 4));  // docs.txt is below 4 GiB
        // Sparse hits of each 8192-document block are packed as (term << 13 |
        // local document), radix-sorted by term (stably, so documents stay
        // ascending) and kept as compact 16-bit runs.
        constexpr uint32_t BLOCK = 8192;
        struct Block {
            uint32_t base;
            std::vector<uint32_t> ids, ends;
            std::vector<uint16_t> docs;
        };
        std::vector<std::vector<Block>> blocks(workers);
        counts.clear();
        threads.run(workers, [&](size_t w) {
            std::vector<uint32_t> hits, at(V);
            uint32_t base = first[w] == NONE ? 0 : first[w];
            auto flush = [&] {
                // Counting sort by term: stable, so documents stay ascending.
                for (uint32_t h : hits) ++at[h >> 13];
                Block& blk = blocks[w].emplace_back();
                blk.base = base;
                uint32_t total = 0;
                for (uint32_t id = 0; id < V; ++id)
                    if (uint32_t k = at[id]) blk.ids.push_back(id), at[id] = total, blk.ends.push_back(total += k);
                blk.docs.resize(total);
                for (uint32_t h : hits) blk.docs[at[h >> 13]++] = static_cast<uint16_t>(h & (BLOCK - 1));
                for (uint32_t id : blk.ids) at[id] = 0;
                blk.ids.shrink_to_fit(), blk.ends.shrink_to_fit();
                hits.clear();
            };
            scan(w, SIZE_MAX, [&](uint32_t doc, const char* line, uint32_t id) {
                if (id == NONE) {
                    if (doc - base == BLOCK) flush(), base = doc;
                    offsets[doc] = static_cast<uint32_t>(line - docs);
                } else if (bitmap_of[id] != NONE) bitmaps[bitmap_of[id] * words + doc / 64] |= 1ull << (doc % 64);
                else hits.push_back(id << 13 | (doc - base));
            });
            if (!hits.empty()) flush();
        });
        // Document frequencies: run lengths for lists, popcounts for bitmaps.
        for (auto& list : blocks)
            for (auto& blk : list)
                for (size_t r = 0; r < blk.ids.size(); ++r) df[blk.ids[r]] += blk.ends[r] - (r ? blk.ends[r - 1] : 0);
        for (size_t t = 0; t < V; ++t)
            if (bitmap_of[t] != NONE)
                for (size_t k = 0; k < words; ++k) df[t] += static_cast<uint32_t>(__builtin_popcountll(bitmaps[bitmap_of[t] * words + k]));
        // Per term, its runs in document order: (block << 20 | run).
        std::vector<const Block*> runs_of;
        for (auto& list : blocks) for (auto& blk : list) runs_of.push_back(&blk);
        std::vector<uint32_t> start(V + 1);
        for (const Block* blk : runs_of) for (uint32_t id : blk->ids) ++start[id + 1];
        for (size_t t = 0; t < V; ++t) start[t + 1] += start[t];
        std::vector<uint32_t> refs(start[V] + 1), cursor(start.begin(), start.end() - 1);
        for (uint32_t k = 0; k < runs_of.size(); ++k)
            for (uint32_t r = 0; r < runs_of[k]->ids.size(); ++r) refs[cursor[runs_of[k]->ids[r]]++] = k << 20 | r;
        std::vector<uint32_t>().swap(cursor);
        offsets[N] = static_cast<uint32_t>(dsize);

        // Answers are formatted into private slots and written in query order.
        std::vector<std::string> answers(queries.size());
        std::atomic<size_t> next{0};
        auto dense = [&](uint32_t t) { return t != NONE && bitmap_of[t] != NONE; };
        auto bits_of = [&](uint32_t t) { return bitmaps + size_t{bitmap_of[t]} * words; };
        auto test = [](const uint64_t* b, uint32_t d) { return (b[d / 64] >> (d % 64)) & 1; };
        auto materialize = [&](uint32_t t, std::vector<uint32_t>& out) {
            out.resize(t == NONE ? 0 : df[t]);
            uint32_t* o = out.data();
            if (t != NONE)
                for (uint32_t k = start[t]; k < start[t + 1]; ++k) {
                    const Block& blk = *runs_of[refs[k] >> 20];
                    uint32_t r = refs[k] & ((1u << 20) - 1);
                    for (uint32_t i = r ? blk.ends[r - 1] : 0; i < blk.ends[r]; ++i) *o++ = blk.base + blk.docs[i];
                }
        };
        // Branch-free merges of sorted lists: AND, OR and NOT in one loop.
        auto merge = [](const std::vector<uint32_t>& a, const std::vector<uint32_t>& b, char op, std::vector<uint32_t>& out) {
            out.resize(a.size() + b.size());
            size_t i = 0, j = 0, k = 0;
            while (i < a.size() && j < b.size()) {
                uint32_t x = a[i], y = b[j];
                out[k] = op == 'O' ? std::min(x, y) : x;
                k += op == 'A' ? x == y : op == 'O' ? 1 : x < y;
                i += x <= y;
                j += y <= x;
            }
            if (op != 'A') for (; i < a.size(); ++i) out[k++] = a[i];
            if (op == 'O') for (; j < b.size(); ++j) out[k++] = b[j];
            out.resize(k);
        };
        // Membership without materializing: find the block run, then the document.
        auto contains = [&](uint32_t t, uint32_t d) -> bool {
            if (t == NONE) return false;
            if (bitmap_of[t] != NONE) return test(bits_of(t), d);
            const uint32_t *b = refs.data() + start[t], *e = refs.data() + start[t + 1];
            auto it = std::upper_bound(b, e, d, [&](uint32_t x, uint32_t ref) { return x < runs_of[ref >> 20]->base; });
            if (it == b) return false;
            const Block& blk = *runs_of[*--it >> 20];
            uint32_t r = *it & ((1u << 20) - 1);
            if (d - blk.base >= BLOCK) return false;
            const uint16_t *lo = blk.docs.data() + (r ? blk.ends[r - 1] : 0), *hi = blk.docs.data() + blk.ends[r];
            return std::binary_search(lo, hi, static_cast<uint16_t>(d - blk.base));
        };
        threads.run(workers, [&](size_t) {
            std::vector<uint32_t> list, other, merged, found;
            std::vector<uint64_t> own(words), spare(words);
            for (size_t qi; (qi = next.fetch_add(16)) < queries.size();)
                for (size_t q = qi; q < std::min(qi + 16, queries.size()); ++q) {
                    const Query& query = queries[q];
                    const std::span<const uint32_t> terms(all_terms.data() + query.at, query.n);
                    const char* ops = all_ops.data() + query.at;  // ops[i] joins terms[i] and terms[i + 1]
                    uint64_t count = 0, sum = 0;
                    uint32_t smallest[10], nsmall = 0;
                    auto report = [&](const std::vector<uint32_t>& l) {
                        count = l.size();
                        for (uint32_t d : l) sum += d;
                        for (; nsmall < 10 && nsmall < l.size(); ++nsmall) smallest[nsmall] = l[nsmall];
                    };
                    if (!query.phrase) {
                        // The running set is a sorted list or a bitmap (`bits`, which
                        // may still point at a term's own bitmap until first written).
                        const uint64_t* bits = nullptr;
                        uint32_t t0 = terms[0];
                        if (dense(t0)) bits = bits_of(t0);
                        else materialize(t0, list);
                        auto writable = [&] {
                            uint64_t* out = bits == own.data() ? spare.data() : own.data();
                            return out;
                        };
                        for (size_t i = 0; i + 1 < terms.size(); ++i) {
                            uint32_t t = terms[i + 1];
                            char op = ops[i];
                            if (bits && dense(t)) {
                                const uint64_t* b = bits_of(t);
                                uint64_t* out = writable();
                                if (op == 'A') for (size_t k = 0; k < words; ++k) out[k] = bits[k] & b[k];
                                else if (op == 'O') for (size_t k = 0; k < words; ++k) out[k] = bits[k] | b[k];
                                else for (size_t k = 0; k < words; ++k) out[k] = bits[k] & ~b[k];
                                bits = out;
                            } else if (bits) {
                                materialize(t, other);
                                if (op == 'A') {
                                    list.clear();
                                    for (uint32_t x : other) if (test(bits, x)) list.push_back(x);
                                    bits = nullptr;
                                } else {
                                    if (bits != own.data()) std::copy(bits, bits + words, own.data()), bits = own.data();
                                    for (uint32_t x : other)
                                        if (op == 'O') own[x / 64] |= 1ull << (x % 64);
                                        else own[x / 64] &= ~(1ull << (x % 64));
                                }
                            } else if (dense(t)) {
                                const uint64_t* b = bits_of(t);
                                if (op == 'O') {
                                    std::copy(b, b + words, own.data());
                                    for (uint32_t x : list) own[x / 64] |= 1ull << (x % 64);
                                    bits = own.data();
                                } else {
                                    uint64_t keep = op == 'A';
                                    size_t n = 0;
                                    for (uint32_t x : list) if (test(b, x) == keep) list[n++] = x;
                                    list.resize(n);
                                }
                            } else if (op != 'O' && t != NONE && list.size() * 32 < df[t]) {
                                bool keep = op == 'A';
                                size_t n = 0;
                                for (uint32_t x : list) if (contains(t, x) == keep) list[n++] = x;
                                list.resize(n);
                            } else {
                                materialize(t, other);
                                if (op == 'A') merge(list, other, 'A', merged);
                                else if (op == 'O') merge(list, other, 'O', merged);
                                else merge(list, other, 'N', merged);
                                list.swap(merged);
                            }
                        }
                        if (bits) {
                            // Exact id sums from six bit-position popcounts per word.
                            static constexpr uint64_t M[6] = {0xAAAAAAAAAAAAAAAAull, 0xCCCCCCCCCCCCCCCCull, 0xF0F0F0F0F0F0F0F0ull,
                                                              0xFF00FF00FF00FF00ull, 0xFFFF0000FFFF0000ull, 0xFFFFFFFF00000000ull};
                            for (size_t k = 0; k < words; ++k) {
                                uint64_t x = bits[k];
                                if (!x) continue;
                                uint64_t c = static_cast<uint64_t>(__builtin_popcountll(x));
                                count += c;
                                sum += c * 64 * k;
                                for (int j = 0; j < 6; ++j) sum += static_cast<uint64_t>(__builtin_popcountll(x & M[j])) << j;
                                for (; x && nsmall < 10; x &= x - 1) smallest[nsmall++] = static_cast<uint32_t>(k * 64 + __builtin_ctzll(x));
                            }
                        } else report(list);
                    } else {
                        // Candidates hold every term; the rarest term seeds them.
                        bool absent = false;
                        uint32_t seed = NONE;
                        size_t at = 0;
                        for (size_t j = 0; j < terms.size(); ++j) {
                            uint32_t t = terms[j];
                            if (t == NONE || df[t] == 0) absent = true;
                            else if (seed == NONE || df[t] < df[seed]) seed = t, at = j;
                        }
                        found.clear();
                        if (!absent) {
                            if (dense(seed)) {
                                list.clear();
                                for (size_t k = 0; k < words; ++k)
                                    for (uint64_t x = bits_of(seed)[k]; x; x &= x - 1)
                                        list.push_back(static_cast<uint32_t>(k * 64 + __builtin_ctzll(x)));
                            } else materialize(seed, list);
                            std::vector<const char*> keys;
                            std::vector<uint32_t> lens;
                            for (uint32_t t : terms) keys.push_back(reinterpret_cast<const char*>(&vocab.keys[t])), lens.push_back(vocab.lens[t]);
                            for (uint32_t d : list) {
                                bool all = true;
                                for (uint32_t t : terms) if (t != seed && !(all = contains(t, d))) break;
                                if (!all) continue;
                                const char* t = static_cast<const char*>(memchr(docs + offsets[d], '\t', 12)) + 1;
                                if (phrase_in(t, static_cast<size_t>(docs + offsets[d + 1] - 1 - t), keys, lens, at)) found.push_back(d);
                            }
                        }
                        report(found);
                    }
                    std::string& out = answers[q];
                    out = std::to_string(count) + ' ' + std::to_string(sum);
                    for (uint32_t i = 0; i < nsmall; ++i) out += ' ' + std::to_string(smallest[i]);
                    out += '\n';
                }
        });
        std::string all;
        for (auto& a : answers) all += a;
        int fd = open((out_dir + "/results.txt").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) throw std::runtime_error("cannot create results.txt");
        for (size_t off = 0; off < all.size();) {
            ssize_t k = write(fd, all.data() + off, all.size() - off);
            if (k <= 0) throw std::runtime_error("write failed");
            off += static_cast<size_t>(k);
        }
        return close(fd) == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
