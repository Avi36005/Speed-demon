#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

#if defined(__x86_64__)
#include <immintrin.h>
#endif

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

template<class Callback>
#if defined(__x86_64__)
__attribute__((target("sse2")))
#endif
void tokenize(std::string_view text, Callback callback) {
    char token[32];  // frozen maximum token length, in bytes
    size_t length = 0;
    size_t offset = 0;
#if defined(__x86_64__)
    // Fold ASCII capitals and classify 16 bounded bytes at once. Bytes with
    // the high bit set fail the signed range tests and stay separators.
    for (; offset + 16 <= text.size(); offset += 16) {
        __m128i bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(text.data() + offset));
        __m128i capitals = _mm_and_si128(_mm_cmpgt_epi8(bytes, _mm_set1_epi8('A' - 1)),
                                          _mm_cmpgt_epi8(_mm_set1_epi8('Z' + 1), bytes));
        bytes = _mm_or_si128(bytes, _mm_and_si128(capitals, _mm_set1_epi8(32)));
        __m128i letters = _mm_and_si128(_mm_cmpgt_epi8(bytes, _mm_set1_epi8('a' - 1)),
                                         _mm_cmpgt_epi8(_mm_set1_epi8('z' + 1), bytes));
        __m128i digits = _mm_and_si128(_mm_cmpgt_epi8(bytes, _mm_set1_epi8('0' - 1)),
                                        _mm_cmpgt_epi8(_mm_set1_epi8('9' + 1), bytes));
        uint32_t mask = static_cast<uint32_t>(_mm_movemask_epi8(_mm_or_si128(letters, digits)));
        alignas(16) char lowered[16];
        _mm_store_si128(reinterpret_cast<__m128i*>(lowered), bytes);
        if (length && !(mask & 1)) {
            callback(std::string_view(token, length));
            length = 0;
        }
        while (mask) {
            unsigned first = std::countr_zero(mask);
            unsigned run = std::countr_one(mask >> first);
            std::memcpy(token + length, lowered + first, run);
            length += run;
            unsigned end = first + run;
            if (end < 16) {
                callback(std::string_view(token, length));
                length = 0;
                mask &= ~((uint32_t{1} << end) - 1);
            } else break;
        }
    }
#endif
    for (; offset < text.size(); ++offset) {
        unsigned char byte = text[offset];
        if (byte >= 'A' && byte <= 'Z') byte += 'a' - 'A';
        if ((byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9')) {
            token[length++] = static_cast<char>(byte);
        } else if (length) {
            callback(std::string_view(token, length));
            length = 0;
        }
    }
    if (length) callback(std::string_view(token, length));
}

struct Query {
    bool phrase;
    std::vector<uint32_t> terms;
    std::vector<char> ops;
    std::vector<size_t> prefix;
};

// Query-backed keys remain mapped throughout the run. A compact probing table
// avoids a separately allocated hash node for every corpus-token lookup.
// Fingerprints only filter probes: complete byte equality resolves collisions.
class Vocabulary {
    struct Slot { uint32_t fingerprint = 0, id = 0; };
    std::vector<Slot> slots = std::vector<Slot>(1024);
    std::vector<std::string_view> keys;
    size_t locate(std::string_view key, size_t hash) const {
        size_t pos = hash & (slots.size() - 1);
        uint32_t fingerprint = static_cast<uint32_t>(hash >> 32);
        while (slots[pos].id &&
               (slots[pos].fingerprint != fingerprint || keys[slots[pos].id - 1] != key))
            pos = (pos + 1) & (slots.size() - 1);
        return pos;
    }
public:
    size_t size() const { return keys.size(); }
    std::string_view word(uint32_t id) const { return keys[id]; }
    uint32_t find(std::string_view key) const {
        auto id = slots[locate(key, std::hash<std::string_view>{}(key))].id;
        return id ? id - 1 : UINT32_MAX;
    }
    uint32_t intern(std::string_view key) {
        if ((keys.size() + 1) * 2 > slots.size()) {
            slots.assign(slots.size() * 2, Slot{});
            for (uint32_t id = 0; id < keys.size(); ++id) {
                size_t hash = std::hash<std::string_view>{}(keys[id]);
                slots[locate(keys[id], hash)] = {static_cast<uint32_t>(hash >> 32), id + 1};
            }
        }
        size_t hash = std::hash<std::string_view>{}(key);
        auto& slot = slots[locate(key, hash)];
        if (!slot.id) {
            keys.push_back(key);
            slot = {static_cast<uint32_t>(hash >> 32), static_cast<uint32_t>(keys.size())};
        }
        return slot.id - 1;
    }
};
using Posting = std::vector<uint32_t>;

// Local document ids increase monotonically. A 16-bit gap stores the
// ordinary case; the escape carries the full uint32 gap without truncation.
template<class Callback>
void local_postings(const std::vector<uint16_t>& gaps, uint32_t first, Callback callback) {
    uint32_t doc = first;
    for (size_t i = 0; i < gaps.size(); ++i) {
        uint32_t gap = gaps[i];
        if (gap == UINT16_MAX) {
            gap = uint32_t(gaps[i + 1]) | (uint32_t(gaps[i + 2]) << 16);
            i += 2;
        }
        doc += gap;
        callback(doc);
    }
}

struct ThreadGroup {
    std::vector<std::thread> values;
    void join() { for (auto& thread : values) if (thread.joinable()) thread.join(); }
    ~ThreadGroup() { join(); }
};

bool phrase_matches(std::string_view text, const Query& query, const Vocabulary& vocabulary) {
    size_t matched = 0;
    bool found = false;
    tokenize(text, [&](std::string_view token) {
        if (found) return;
        // Exact streaming KMP preserves every token position, including tokens
        // absent from the query vocabulary. No document sequence is allocated.
        while (matched && token != vocabulary.word(query.terms[matched]))
            matched = query.prefix[matched - 1];
        if (token == vocabulary.word(query.terms[matched])) ++matched;
        if (matched == query.terms.size()) found = true;
    });
    return found;
}

struct Summary {
    uint64_t count = 0, sum = 0;
    std::array<uint32_t, 10> first{};
    void add(uint32_t id) {
        if (count < first.size()) first[count] = id;
        ++count;
        sum += id;
    }
};

#if defined(__x86_64__)
__attribute__((target("popcnt")))
#endif
Summary summarize_bits(const std::vector<uint64_t>& bitmap) {
    Summary result;
    for (size_t word = 0; word < bitmap.size(); ++word) {
        uint64_t bits = bitmap[word];
        uint64_t count = std::popcount(bits);
        // The six masks sum the six binary digits of every set-bit position.
        // This computes exact id sums without materializing a dense posting.
        result.sum += word * 64 * count;
        result.sum += std::popcount(bits & 0xaaaaaaaaaaaaaaaaULL);
        result.sum += 2ULL * std::popcount(bits & 0xccccccccccccccccULL);
        result.sum += 4ULL * std::popcount(bits & 0xf0f0f0f0f0f0f0f0ULL);
        result.sum += 8ULL * std::popcount(bits & 0xff00ff00ff00ff00ULL);
        result.sum += 16ULL * std::popcount(bits & 0xffff0000ffff0000ULL);
        result.sum += 32ULL * std::popcount(bits & 0xffffffff00000000ULL);
        uint64_t ids = bits;
        size_t first = std::min(result.count, uint64_t{10});
        while (ids && first < result.first.size()) {
            result.first[first++] = static_cast<uint32_t>(word * 64 + std::countr_zero(ids));
            ids &= ids - 1;
        }
        result.count += count;
    }
    return result;
}

Summary evaluate(const Query& query, const std::vector<Posting>& index,
                 const std::vector<std::string_view>& documents, const Vocabulary& vocabulary,
                 const std::vector<std::vector<uint64_t>>& bitmaps,
                 const std::vector<uint32_t>& frequencies) {
    if (query.phrase) {
        auto anchor = *std::min_element(query.terms.begin(), query.terms.end(), [&](auto a, auto b) {
            return frequencies[a] < frequencies[b];
        });
        Summary result;
        auto consider = [&](uint32_t doc) {
            for (auto term : query.terms) {
                const auto& bits = bitmaps[term];
                bool present = bits.empty()
                    ? std::binary_search(index[term].begin(), index[term].end(), doc)
                    : ((bits[doc / 64] >> (doc % 64)) & 1) != 0;
                if (!present) return;
            }
            if (phrase_matches(documents[doc], query, vocabulary)) result.add(doc);
        };
        if (bitmaps[anchor].empty()) {
            for (auto doc : index[anchor]) consider(doc);
        } else {
            for (size_t word = 0; word < bitmaps[anchor].size(); ++word) {
                uint64_t bits = bitmaps[anchor][word];
                while (bits) {
                    consider(static_cast<uint32_t>(word * 64 + std::countr_zero(bits)));
                    bits &= bits - 1;
                }
            }
        }
        return result;
    }
    size_t words = (documents.size() + 63) / 64;
    size_t threshold = std::max(size_t{64}, documents.size() / 64);
    bool dense = !bitmaps[query.terms[0]].empty();
    std::vector<uint64_t> bits = bitmaps[query.terms[0]];
    Posting result = dense ? Posting{} : index[query.terms[0]], next;
    auto contains = [](const auto& bitmap, uint32_t id) {
        return (bitmap[id / 64] >> (id % 64)) & 1;
    };
    auto make_dense = [&] {
        bits.assign(words, 0);
        for (auto id : result) bits[id / 64] |= uint64_t{1} << (id % 64);
        dense = true;
        result.clear();
    };
    for (size_t i = 0; i < query.ops.size(); ++i) {
        const auto& rhs = index[query.terms[i + 1]];
        const auto& rhs_bits = bitmaps[query.terms[i + 1]];
        char op = query.ops[i];
        next.clear();
        if (!dense && op == 'O' && !rhs_bits.empty()) make_dense();
        if (dense && !rhs_bits.empty()) {
            for (size_t w = 0; w < words; ++w) {
                if (op == 'A') bits[w] &= rhs_bits[w];
                else if (op == 'O') bits[w] |= rhs_bits[w];
                else bits[w] &= ~rhs_bits[w];
            }
            continue;
        }
        if (dense) {
            if (op == 'A') {
                for (auto id : rhs) if (contains(bits, id)) next.push_back(id);
                dense = false;
                result.swap(next);
            } else {
                for (auto id : rhs) {
                    uint64_t mask = uint64_t{1} << (id % 64);
                    if (op == 'O') bits[id / 64] |= mask;
                    else bits[id / 64] &= ~mask;
                }
            }
            continue;
        }
        if (!rhs_bits.empty()) {
            for (auto id : result)
                if (contains(rhs_bits, id) == (op == 'A')) next.push_back(id);
        } else if (op == 'A') {
            const Posting* small = &result;
            const Posting* large = &rhs;
            if (small->size() > large->size()) std::swap(small, large);
            if (small->size() * 16 < large->size()) {
                for (auto id : *small)
                    if (std::binary_search(large->begin(), large->end(), id)) next.push_back(id);
            } else
                std::set_intersection(result.begin(), result.end(), rhs.begin(), rhs.end(), std::back_inserter(next));
        } else if (op == 'O')
            std::set_union(result.begin(), result.end(), rhs.begin(), rhs.end(), std::back_inserter(next));
        else
            std::set_difference(result.begin(), result.end(), rhs.begin(), rhs.end(), std::back_inserter(next));
        result.swap(next);
        if (result.size() > threshold) make_dense();
    }
    if (dense) return summarize_bits(bits);
    Summary summary;
    for (uint32_t id : result) summary.add(id);
    return summary;
}

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        size_t worker_count = 6;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (i + 1 < argc && arg == "--data") data_dir = argv[++i];
            else if (i + 1 < argc && arg == "--out") out_dir = argv[++i];
            else if (i + 1 < argc && arg == "--workers") worker_count = std::stoul(argv[++i]);
            else throw std::runtime_error("usage: solution [--data DIR] [--out DIR] [--workers 1..12]");
        }
        if (worker_count < 1 || worker_count > 12) throw std::runtime_error("workers must be 1..12");
        Input query_file(data_dir + "/queries.txt");
        Vocabulary vocabulary;
        std::vector<Query> queries;
        const char* p = query_file.data;
        const char* end = p + query_file.size;
        while (p < end) {
            const char* start = p;
            p = static_cast<const char*>(std::memchr(p, '\n', end - p));
            std::string_view line(start, p++ - start);
            Query query{line.starts_with("PHRASE"), {}, {}, {}};
            auto words = query.phrase ? line.substr(8, line.size() - 9) : line.substr(5);
            bool term = true;
            while (!words.empty()) {
                size_t split = words.find(' ');
                auto word = words.substr(0, split);
                if (term) {
                    query.terms.push_back(vocabulary.intern(word));
                } else query.ops.push_back(word[0]);
                if (!query.phrase) term = !term;
                if (split == std::string_view::npos) break;
                words.remove_prefix(split + 1);
            }
            if (query.phrase) {
                query.prefix.resize(query.terms.size(), 0);
                for (size_t i = 1, j = 0; i < query.terms.size(); ++i) {
                    while (j && query.terms[i] != query.terms[j]) j = query.prefix[j - 1];
                    if (query.terms[i] == query.terms[j]) ++j;
                    query.prefix[i] = j;
                }
            }
            queries.push_back(std::move(query));
        }
        std::vector<Posting> index(vocabulary.size());
        Input corpus(data_dir + "/docs.txt");
        std::vector<std::string_view> documents;
        p = corpus.data;
        end = p + corpus.size;
        while (p < end) {
            while (*p++ != '\t') {}  // ids are dense and ascending
            const char* start = p;
            p = static_cast<const char*>(std::memchr(p, '\n', end - p));
            std::string_view text(start, p++ - start);
            documents.push_back(text);
        }
        // Each document range starts on a bitmap word boundary. A private
        // term promotes once its bitmap is smaller than its postings; common
        // terms never retain a second full posting representation.
        struct LocalTerm {
            std::vector<uint16_t> sparse;
            std::vector<uint64_t> dense;
            uint32_t frequency = 0, last = UINT32_MAX;
        };
        size_t words = (documents.size() + 63) / 64;
        size_t threshold = std::max(size_t{64}, documents.size() / 64);
        std::vector<std::vector<LocalTerm>> partial(worker_count, std::vector<LocalTerm>(index.size()));
        const Vocabulary& needed = vocabulary;
        auto build_index = [&](size_t worker) {
            auto& local = partial[worker];
            size_t first_word = words * worker / worker_count;
            size_t last_word = words * (worker + 1) / worker_count;
            size_t first = first_word * 64;
            size_t last = std::min(documents.size(), last_word * 64);
            size_t local_threshold = std::max(size_t{16}, (last - first) / 64);
            for (size_t doc = first; doc < last; ++doc) {
                tokenize(documents[doc], [&](std::string_view token) {
                    uint32_t term = needed.find(token);
                    if (term == UINT32_MAX) return;
                    auto& value = local[term];
                    if (value.last == doc) return;
                    uint32_t previous = value.last == UINT32_MAX ? static_cast<uint32_t>(first) : value.last;
                    value.last = static_cast<uint32_t>(doc);
                    ++value.frequency;
                    if (value.dense.empty()) {
                        uint32_t gap = static_cast<uint32_t>(doc) - previous;
                        if (gap < UINT16_MAX) value.sparse.push_back(static_cast<uint16_t>(gap));
                        else {
                            value.sparse.push_back(UINT16_MAX);
                            value.sparse.push_back(static_cast<uint16_t>(gap));
                            value.sparse.push_back(static_cast<uint16_t>(gap >> 16));
                        }
                        if (value.frequency > local_threshold) {
                            value.dense.resize(last_word - first_word, 0);
                            local_postings(value.sparse, static_cast<uint32_t>(first), [&](uint32_t id) {
                                value.dense[id / 64 - first_word] |= uint64_t{1} << (id % 64);
                            });
                            std::vector<uint16_t>{}.swap(value.sparse);
                        }
                    } else value.dense[doc / 64 - first_word] |= uint64_t{1} << (doc % 64);
                });
            }
        };
        ThreadGroup threads;
        for (size_t worker = 1; worker < worker_count; ++worker)
            threads.values.emplace_back(build_index, worker);
        build_index(0);
        threads.join();
        std::vector<std::vector<uint64_t>> bitmaps(index.size());
        std::vector<uint32_t> frequencies(index.size(), 0);
        std::atomic<size_t> next_term{0};
        auto merge_terms = [&] {
            for (;;) {
                size_t term = next_term.fetch_add(1, std::memory_order_relaxed);
                if (term >= index.size()) break;
                uint32_t total = 0;
                for (const auto& local : partial) total += local[term].frequency;
                frequencies[term] = total;
                bool dense = total > threshold;
                if (dense) bitmaps[term].resize(words, 0);
                else index[term].reserve(total);
                for (size_t worker = 0; worker < worker_count; ++worker) {
                    auto& value = partial[worker][term];
                    size_t first_word = words * worker / worker_count;
                    if (value.dense.empty()) {
                        local_postings(value.sparse, static_cast<uint32_t>(first_word * 64), [&](uint32_t id) {
                            if (dense) bitmaps[term][id / 64] |= uint64_t{1} << (id % 64);
                            else index[term].push_back(id);
                        });
                    } else if (dense) {
                        std::copy(value.dense.begin(), value.dense.end(), bitmaps[term].begin() + first_word);
                    } else {
                        for (size_t word = 0; word < value.dense.size(); ++word) {
                            uint64_t bits = value.dense[word];
                            while (bits) {
                                index[term].push_back(static_cast<uint32_t>((first_word + word) * 64 + std::countr_zero(bits)));
                                bits &= bits - 1;
                            }
                        }
                    }
                    // The term is complete globally; release its private
                    // storage before allocating the next merged term.
                    std::vector<uint16_t>{}.swap(value.sparse);
                    std::vector<uint64_t>{}.swap(value.dense);
                }
            }
        };
        threads.values.clear();
        for (size_t worker = 1; worker < worker_count; ++worker)
            threads.values.emplace_back(merge_terms);
        merge_terms();
        threads.join();
        partial.clear();
        // Queries read immutable indexes and own their temporary sets. Output
        // slots are disjoint; the final write preserves original query order.
        std::vector<std::string> answers(queries.size());
        std::atomic<size_t> next_query{0};
        auto answer_queries = [&] {
            while (true) {
                size_t first = next_query.fetch_add(16, std::memory_order_relaxed);
                if (first >= queries.size()) break;
                for (size_t q = first; q < std::min(first + 16, queries.size()); ++q) {
                    Summary result = evaluate(queries[q], index, documents, vocabulary, bitmaps, frequencies);
                    auto& answer = answers[q];
                    answer = std::to_string(result.count) + ' ' + std::to_string(result.sum);
                    for (size_t i = 0; i < std::min(uint64_t{10}, result.count); ++i)
                        answer += ' ' + std::to_string(result.first[i]);
                    answer += '\n';
                }
            }
        };
        threads.values.clear();
        for (size_t worker = 1; worker < worker_count; ++worker)
            threads.values.emplace_back(answer_queries);
        answer_queries();
        threads.join();
        std::ofstream out(out_dir + "/results.txt", std::ios::binary);
        for (const auto& answer : answers) out.write(answer.data(), answer.size());
        if (!out) throw std::runtime_error("output write failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
