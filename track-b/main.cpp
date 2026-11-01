#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
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
#include <unordered_map>
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

template<class Callback>
void tokenize(std::string_view text, Callback callback) {
    char token[32];  // frozen maximum token length, in bytes
    size_t length = 0;
    for (unsigned char byte : text) {
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
};

using Vocabulary = std::unordered_map<std::string_view, uint32_t>;
using Posting = std::vector<uint32_t>;

struct ThreadGroup {
    std::vector<std::thread> values;
    void join() { for (auto& thread : values) if (thread.joinable()) thread.join(); }
    ~ThreadGroup() { join(); }
};

bool phrase_matches(std::string_view text, const Query& query, const Vocabulary& vocabulary) {
    Posting sequence;
    tokenize(text, [&](std::string_view token) {
        auto it = vocabulary.find(token);
        // Unneeded tokens must still occupy positions: dropping one would invent adjacency.
        sequence.push_back(it == vocabulary.end() ? UINT32_MAX : it->second);
    });
    return std::search(sequence.begin(), sequence.end(), query.terms.begin(), query.terms.end())
           != sequence.end();
}

Posting evaluate(const Query& query, const std::vector<Posting>& index,
                 const std::vector<std::string_view>& documents, const Vocabulary& vocabulary,
                 const std::vector<std::vector<uint64_t>>& bitmaps) {
    if (query.phrase) {
        auto anchor = *std::min_element(query.terms.begin(), query.terms.end(), [&](auto a, auto b) {
            return index[a].size() < index[b].size();
        });
        Posting result;
        for (auto doc : index[anchor]) {
            bool present = true;
            for (auto term : query.terms) {
                if (!std::binary_search(index[term].begin(), index[term].end(), doc)) {
                    present = false;
                    break;
                }
            }
            if (present && phrase_matches(documents[doc], query, vocabulary)) result.push_back(doc);
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
    if (dense) {
        result.clear();
        for (size_t w = 0; w < bits.size(); ++w) {
            uint64_t value = bits[w];
            while (value) {
                result.push_back(static_cast<uint32_t>(w * 64 + std::countr_zero(value)));
                value &= value - 1;
            }
        }
    }
    return result;
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
            while (*p != '\n') ++p;
            std::string_view line(start, p++ - start);
            Query query{line.starts_with("PHRASE"), {}, {}};
            auto words = query.phrase ? line.substr(8, line.size() - 9) : line.substr(5);
            bool term = true;
            while (!words.empty()) {
                size_t split = words.find(' ');
                auto word = words.substr(0, split);
                if (term) {
                    auto [it, inserted] = vocabulary.try_emplace(word, static_cast<uint32_t>(vocabulary.size()));
                    query.terms.push_back(it->second);
                } else query.ops.push_back(word[0]);
                if (!query.phrase) term = !term;
                if (split == std::string_view::npos) break;
                words.remove_prefix(split + 1);
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
            while (*p != '\n') ++p;
            std::string_view text(start, p++ - start);
            documents.push_back(text);
        }
        // Documents are complete views before workers begin. Each worker owns
        // its postings and an ordered range of document IDs. Concatenating ranges
        // in worker order therefore preserves sorted, duplicate-free postings.
        std::vector<std::vector<Posting>> partial(worker_count - 1, std::vector<Posting>(index.size()));
        const Vocabulary& needed = vocabulary;
        auto build_index = [&](size_t worker) {
            auto& local = worker == 0 ? index : partial[worker - 1];
            size_t first = documents.size() * worker / worker_count;
            size_t last = documents.size() * (worker + 1) / worker_count;
            for (size_t doc = first; doc < last; ++doc) {
                tokenize(documents[doc], [&](std::string_view token) {
                    auto it = needed.find(token);
                    if (it == needed.end()) return;
                    auto& posting = local[it->second];
                    if (posting.empty() || posting.back() != doc) posting.push_back(static_cast<uint32_t>(doc));
                });
            }
        };
        ThreadGroup threads;
        for (size_t worker = 1; worker < worker_count; ++worker)
            threads.values.emplace_back(build_index, worker);
        build_index(0);
        threads.join();
        for (size_t term = 0; term < index.size(); ++term) {
            auto& posting = index[term];
            size_t total = posting.size();
            for (const auto& local : partial) total += local[term].size();
            posting.reserve(total);
            for (const auto& local : partial)
                posting.insert(posting.end(), local[term].begin(), local[term].end());
        }
        partial.clear();
        // Dense terms receive compact bitmaps; sparse terms retain just postings.
        // Keeping sorted postings also gives phrase anchors without bitmap scans.
        std::vector<std::vector<uint64_t>> bitmaps(index.size());
        size_t threshold = std::max(size_t{64}, documents.size() / 64);
        for (size_t term = 0; term < index.size(); ++term) {
            if (index[term].size() <= threshold) continue;
            auto& bits = bitmaps[term];
            bits.resize((documents.size() + 63) / 64, 0);
            for (auto id : index[term]) bits[id / 64] |= uint64_t{1} << (id % 64);
        }
        // Queries read immutable indexes and own their temporary sets. Output
        // slots are disjoint; the final write preserves original query order.
        std::vector<std::string> answers(queries.size());
        std::atomic<size_t> next_query{0};
        auto answer_queries = [&] {
            while (true) {
                size_t first = next_query.fetch_add(16, std::memory_order_relaxed);
                if (first >= queries.size()) break;
                for (size_t q = first; q < std::min(first + 16, queries.size()); ++q) {
                    Posting result = evaluate(queries[q], index, documents, vocabulary, bitmaps);
                    uint64_t sum = std::accumulate(result.begin(), result.end(), uint64_t{0});
                    auto& answer = answers[q];
                    answer = std::to_string(result.size()) + ' ' + std::to_string(sum);
                    for (size_t i = 0; i < std::min(size_t{10}, result.size()); ++i)
                        answer += ' ' + std::to_string(result[i]);
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
