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
#include <sys/prctl.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>
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
    // Word loads may read a few bytes before the start or past the end: the
    // mapping sits between anonymous zero pages.
    char* base = static_cast<char*>(mmap(nullptr, size + 12288, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (base == MAP_FAILED || (size && mmap(base + 4096, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED))
        throw std::runtime_error("mmap failed");
    close(fd);
    return base + 4096;
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

        // The corpus is indexed and queried in phases, so the whole index never
        // exists at once. Each phase is split into one range per worker; ranges
        // start at multiples of 64 documents so bitmap words are private.
        const size_t phases = 8, PR = workers * 4, R = phases * PR;  // ranges claimed in turn within a phase
        std::vector<const char*> cut(R + 1, docs + dsize);
        std::vector<uint32_t> first(R + 1, 0);
        cut[0] = docs;
        for (size_t i = 1; i < R; ++i) {
            const char* p = std::max(cut[i - 1], docs + dsize * i / R);
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
        for (size_t i = 0; i < R; ++i) first[i] = std::min(first[i], N);
        first[R] = N;
        size_t words = 0, most = 0;  // bitmap words of the current phase, and of the largest
        uint32_t pbase = 0;
        for (size_t ph = 0; ph < phases; ++ph) most = std::max<size_t>(most, (first[(ph + 1) * PR] - first[ph * PR] + 63) / 64);
        Threads threads;
        struct Token {
            const char* key;
            uint32_t len;
            uint64_t h;
        };
        struct State {  // one per worker, kept across ranges and phases
            std::vector<uint32_t> last, seq, hits, at, seen;  // seq: a document's token ids, NONE for others
            std::vector<uint64_t> touched;
            std::vector<Token> pending;
            Scratch sc;
        };
        std::vector<State> states(workers);
        auto scan = [&](size_t w, State& st, size_t limit, auto&& hit, auto&& whole) {
            auto& last = st.last;
            auto& sc = st.sc;
            auto& pending = st.pending;
            auto& seq = st.seq;
            if (last.empty()) last.assign(V, NONE);
            uint32_t doc = first[w] == NONE ? 0 : first[w];
            for (const char* p = cut[w]; p < cut[w + 1] && limit; ++doc, --limit) {
                const char* t = static_cast<const char*>(memchr(p, '\t', 12)) + 1;
                const char* nl = static_cast<const char*>(memchr(t, '\n', static_cast<size_t>(cut[w + 1] - t)));
                hit(doc, p, NONE);
                // Hash every token first and prefetch its slot, then resolve.
                pending.clear();
                tokens(t, nl, sc, [&](const char* key, uint32_t len) {
                    if (len > 32) return pending.push_back({key, len, 0});
                    uint64_t h = Vocabulary::hash(key, len);
                    __builtin_prefetch(vocab.slot(h));
                    pending.push_back({key, len, h});
                });
                seq.clear();
                for (const Token& k : pending) {
                    uint32_t id = k.len > 32 ? NONE : vocab.find(k.key, k.len, k.h);
                    seq.push_back(id);
                    if (id != NONE && last[id] != doc) last[id] = doc, hit(doc, p, id);
                }
                whole(doc, seq);
                p = nl + 1;
            }
        };
        // A small sample predicts the dense terms; their hits go straight into
        // bitmaps. The prediction only picks a representation, never an answer.
        std::vector<std::vector<uint32_t>> counts(workers, std::vector<uint32_t>(V));
        std::vector<uint32_t> sampled(workers);
        const size_t per = std::max<size_t>(64, 8192 / workers);
        threads.run(workers, [&](size_t w) {
            State own;  // the sample's "last document" marks must not leak into the scan
            scan(w * phases * 4, own, per, [&](uint32_t, const char*, uint32_t id) { id == NONE ? ++sampled[w] : ++counts[w][id]; },
                 [](uint32_t, const std::vector<uint32_t>&) {});
        });
        std::vector<uint32_t> bitmap_of(V, NONE), df(V);
        size_t nbitmaps = 0, total = 0;
        for (size_t w = 0; w < workers; ++w) total += sampled[w];
        std::vector<uint32_t> sdf(V);
        for (size_t t = 0; t < V; ++t) {
            for (size_t w = 0; w < workers; ++w) sdf[t] += counts[w][t], counts[w][t] = 0;
            if (sdf[t] * 40 > total) bitmap_of[t] = static_cast<uint32_t>(nbitmaps++);
        }
        // Phrases are matched during the scan, exactly, by token ids around
        // each occurrence of an anchor: the phrase term rarest in the sample.
        // A phrase with a term no vocabulary holds matches nothing.
        std::vector<uint32_t> phrase_of(queries.size(), NONE), anchor_at(V + 2), anchors, offset;
        std::vector<const Query*> phrases;
        for (size_t q = 0; q < queries.size(); ++q) {
            const Query& query = queries[q];
            if (!query.phrase) continue;
            const uint32_t* t = all_terms.data() + query.at;
            if (std::find(t, t + query.n, NONE) != t + query.n) continue;
            uint32_t j = 0;
            for (uint32_t i = 1; i < query.n; ++i) if (sdf[t[i]] < sdf[t[j]]) j = i;
            phrase_of[q] = static_cast<uint32_t>(phrases.size());
            phrases.push_back(&query);
            offset.push_back(j);
            ++anchor_at[t[j] + 2];
        }
        for (size_t t = 0; t < V; ++t) anchor_at[t + 2] += anchor_at[t + 1];
        std::vector<uint64_t> is_anchor((V + 63) / 64);  // an L1-sized filter before the anchor lists
        for (size_t t = 0; t < V; ++t) if (anchor_at[t + 2] != anchor_at[t + 1]) is_anchor[t / 64] |= 1ull << (t % 64);
        anchors.resize(phrases.size());
        for (uint32_t p = 0; p < phrases.size(); ++p) anchors[anchor_at[all_terms[phrases[p]->at + offset[p]] + 1]++] = p;
        std::vector<std::vector<uint64_t>> matched(R);  // phrase << 32 | document, per range in document order
        auto* bitmaps = static_cast<uint64_t*>(calloc(nbitmaps * most + 1, 8));
        // Sparse hits of each 8192-document block are packed as (term << 13 |
        // local document), radix-sorted by term (stably, so documents stay
        // ascending) and kept as compact 16-bit runs.
        constexpr uint32_t BLOCK = 8192;
        struct Block {
            uint32_t base;
            std::vector<uint32_t> ids, ends;
            std::vector<uint16_t> docs;
        };
        std::vector<std::vector<Block>> blocks(R);  // per range, so ranges in order give document order
        std::atomic<size_t> claim{0};
        counts.clear();
        struct Tally {
            uint64_t count = 0, sum = 0;
            uint32_t n = 0, smallest[10];
        };
        std::vector<Tally> tally(queries.size());
        for (size_t ph = 0; ph < phases; ++ph) {
        pbase = first[ph * PR];
        words = (first[(ph + 1) * PR] - pbase + 63) / 64;
        if (!words) continue;  // a phase without documents adds nothing
        memset(bitmaps, 0, nbitmaps * words * 8);
        std::fill(df.begin(), df.end(), 0);
        for (auto& list : blocks) list.clear();
        claim = ph * PR;
        threads.run(workers, [&](size_t w) {
          State& st = states[w];
          auto& hits = st.hits;
          auto& at = st.at;
          if (at.empty()) at.assign(V, 0), st.seen.assign(phrases.size(), NONE), st.touched.assign(V / 64 + 1, 0);
          auto& touched = st.touched;
          for (size_t r; (r = claim.fetch_add(1)) < (ph + 1) * PR;) {
            uint32_t base = first[r];
            auto flush = [&] {
                // Counting sort by term: stable, so documents stay ascending.
                for (uint32_t h : hits) ++at[h >> 13], touched[h >> 19] |= 1ull << (h >> 13 & 63);
                Block& blk = blocks[r].emplace_back();
                blk.base = base;
                uint32_t total = 0;
                for (size_t i = 0; i < touched.size(); ++i)  // terms in ascending order, via a bit per term
                    for (uint64_t m = std::exchange(touched[i], 0); m; m &= m - 1) {
                        uint32_t id = static_cast<uint32_t>(i * 64 + __builtin_ctzll(m)), k = at[id];
                        blk.ids.push_back(id), at[id] = total, blk.ends.push_back(total += k);
                    }
                blk.docs.resize(total);
                for (uint32_t h : hits) blk.docs[at[h >> 13]++] = static_cast<uint16_t>(h & (BLOCK - 1));
                for (uint32_t id : blk.ids) at[id] = 0;
                blk.ids.shrink_to_fit(), blk.ends.shrink_to_fit();
                hits.clear();
            };
            auto& seen = st.seen;
            auto& mine = matched[r];
            scan(r, st, SIZE_MAX, [&](uint32_t doc, const char*, uint32_t id) {
                if (id == NONE) {
                    if (doc - base == BLOCK) flush(), base = doc;
                } else if (bitmap_of[id] != NONE) bitmaps[bitmap_of[id] * words + (doc - pbase) / 64] |= 1ull << (doc % 64);
                else hits.push_back(id << 13 | (doc - base));
            }, [&](uint32_t doc, const std::vector<uint32_t>& ids) {
                for (size_t i = 0; i < ids.size(); ++i) {
                    if (ids[i] == NONE || !(is_anchor[ids[i] / 64] >> (ids[i] % 64) & 1)) continue;
                    for (uint32_t e = anchor_at[ids[i]]; e < anchor_at[ids[i] + 1]; ++e) {
                        uint32_t ph = anchors[e], j = offset[ph], k = phrases[ph]->n;
                        if (i < j || i - j + k > ids.size() || seen[ph] == doc) continue;
                        const uint32_t* want = all_terms.data() + phrases[ph]->at;
                        if (std::equal(want, want + k, ids.begin() + static_cast<std::ptrdiff_t>(i - j)))
                            seen[ph] = doc, mine.push_back(uint64_t{ph} << 32 | doc);
                    }
                }
            });
            if (!hits.empty()) flush();
            drop(cut[r], cut[r + 1]);  // this range is never read again
          }
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

        // Every BOOL query is answered on this phase and added to its tally.
        std::atomic<size_t> next{0};
        auto dense = [&](uint32_t t) { return t != NONE && bitmap_of[t] != NONE; };
        auto bits_of = [&](uint32_t t) { return bitmaps + size_t{bitmap_of[t]} * words; };
        auto test = [&](const uint64_t* b, uint32_t d) { return (b[(d - pbase) / 64] >> (d % 64)) & 1; };
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
            std::vector<uint64_t> own(words + 1), spare(words + 1);
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
                                        if (op == 'O') own[(x - pbase) / 64] |= 1ull << (x % 64);
                                        else own[(x - pbase) / 64] &= ~(1ull << (x % 64));
                                }
                            } else if (dense(t)) {
                                const uint64_t* b = bits_of(t);
                                if (op == 'O') {
                                    std::copy(b, b + words, own.data());
                                    for (uint32_t x : list) own[(x - pbase) / 64] |= 1ull << (x % 64);
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
                                sum += c * (pbase + 64 * k);
                                for (int j = 0; j < 6; ++j) sum += static_cast<uint64_t>(__builtin_popcountll(x & M[j])) << j;
                                for (; x && nsmall < 10; x &= x - 1) smallest[nsmall++] = static_cast<uint32_t>(pbase + k * 64 + __builtin_ctzll(x));
                            }
                        } else report(list);
                    } else
                        continue;  // phrases are tallied once, after the last phase
                    Tally& t = tally[q];
                    t.count += count, t.sum += sum;
                    for (uint32_t i = 0; i < nsmall && t.n < 10; ++i) t.smallest[t.n++] = smallest[i];
                }
        });
        }  // phases
        // Each phrase's matching documents, ascending: workers in order, then counting by phrase.
        std::vector<uint32_t> phrase_at(phrases.size() + 1), phrase_docs;
        for (auto& list : matched) for (uint64_t m : list) ++phrase_at[(m >> 32) + 1];
        for (size_t ph = 0; ph < phrases.size(); ++ph) phrase_at[ph + 1] += phrase_at[ph];
        phrase_docs.resize(phrase_at.back());
        {
            std::vector<uint32_t> fill(phrase_at.begin(), phrase_at.end() - 1);
            for (auto& list : matched) for (uint64_t m : list) phrase_docs[fill[m >> 32]++] = static_cast<uint32_t>(m);
        }
        std::string all;
        for (size_t q = 0; q < queries.size(); ++q) {
            Tally t = tally[q];
            if (queries[q].phrase && phrase_of[q] != NONE) {
                for (uint32_t i = phrase_at[phrase_of[q]]; i < phrase_at[phrase_of[q] + 1]; ++i, ++t.count) {
                    t.sum += phrase_docs[i];
                    if (t.n < 10) t.smallest[t.n++] = phrase_docs[i];
                }
            }
            all += std::to_string(t.count) + ' ' + std::to_string(t.sum);
            for (uint32_t i = 0; i < t.n; ++i) all += ' ' + std::to_string(t.smallest[i]);
            all += '\n';
        }
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
