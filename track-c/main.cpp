#include <algorithm>
#include <array>
#include <functional>
#include <atomic>
#include <memory>
#include <memory_resource>
#include <thread>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
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

struct Parser {
    const char* p;
    const char* end;
    void bom() {
        if (end - p >= 3 && std::string_view(p, 3) == "\xef\xbb\xbf") p += 3;
    }
    uint64_t integer() {
        uint64_t value = 0;
        while (p < end && *p >= '0' && *p <= '9') value = value * 10 + (*p++ - '0');
        return value;
    }
    // Called only at a field boundary. Quotes and escaped pairs are consumed
    // together; commas, CR, LF and BOM bytes inside quotes remain literal.
    size_t text(std::string* decoded) {
        if (decoded) decoded->clear();
        size_t length = 0;
        if (p < end && *p == '"') {
            ++p;
            while (p < end) {
                // A quoted run has no structural byte except a quote. libc's
                // bounded search skips literal commas/LFs/BOMs together.
                const char* quote = static_cast<const char*>(std::memchr(p, '"', end - p));
                size_t run = quote ? static_cast<size_t>(quote - p) : static_cast<size_t>(end - p);
                if (decoded) decoded->append(p, run);
                length += run;
                p += run;
                if (p == end) break;
                ++p;
                if (p == end || *p != '"') break;
                ++p;
                if (decoded) decoded->push_back('"');
                ++length;
            }
        } else {
            while (p < end && *p != ',' && *p != '\r' && *p != '\n') {
                if (decoded) decoded->push_back(*p);
                ++p;
                ++length;
            }
        }
        return length;
    }
    std::string_view name(std::string& scratch) {
        const char* start = p;
        if (p < end && *p == '"') {
            const char* quote = static_cast<const char*>(std::memchr(p + 1, '"', end - p - 1));
            if (quote && (quote + 1 == end || quote[1] != '"')) {
                p = quote + 1;
                return {start + 1, static_cast<size_t>(quote - start - 1)};
            }
            text(&scratch);
            return scratch;
        }
        while (p < end && *p != ',' && *p != '\r' && *p != '\n') ++p;
        return {start, static_cast<size_t>(p - start)};
    }
    int64_t cents() {
        bool negative = p < end && *p == '-';
        if (negative) ++p;
        int64_t value = static_cast<int64_t>(integer()) * 100;
        ++p;  // decimal point, guaranteed by the grammar
        value += (p[0] - '0') * 10 + p[1] - '0';
        p += 2;
        return negative ? -value : value;
    }
    void terminator() {
        if (p < end && *p == '\r') ++p;
        if (p < end && *p == '\n') ++p;
    }
};

struct Country {
    std::string code;
    uint64_t count = 0, ids = 0, name_bytes = 0, note_bytes = 0;
    int64_t sum = 0, min = std::numeric_limits<int64_t>::max();
    int64_t max = std::numeric_limits<int64_t>::min();
    uint64_t distinct = 0;
    explicit Country(std::string key) : code(std::move(key)) {}
};

// Entries are immutable after release-publication and remain alive until all
// parser threads join. Membership bits only change from zero to one. Thus a
// normal lookup needs no shard mutex, and exactly one fetch_or counts each pair.
class Names {
    struct Entry {
        std::string_view key;
        size_t hash;
        std::atomic<uint64_t>* bits;
        Entry(std::string_view value, size_t h, size_t words, std::pmr::memory_resource& arena)
            : hash(h) {
            char* text = static_cast<char*>(arena.allocate(std::max(value.size(), size_t{1}), alignof(char)));
            std::memcpy(text, value.data(), value.size());
            key = {text, value.size()};
            bits = static_cast<std::atomic<uint64_t>*>(arena.allocate(words * sizeof(*bits), alignof(std::atomic<uint64_t>)));
            for (size_t i = 0; i < words; ++i) std::construct_at(bits + i, uint64_t{0});
        }
    };
    struct Bucket {
        std::array<std::atomic<Entry*>, 4> slots{};
        std::atomic<Bucket*> next{nullptr};
    };
    static constexpr size_t bucket_count = 131072;
    std::unique_ptr<Bucket[]> roots;
    size_t words;
    static bool mark(Entry* entry, uint16_t country) {
        auto& word = entry->bits[country / 64];
        uint64_t mask = uint64_t{1} << (country % 64);
        if (word.load(std::memory_order_relaxed) & mask) return false;
        return !(word.fetch_or(mask, std::memory_order_relaxed) & mask);
    }
public:
    explicit Names(size_t countries)
        : roots(std::make_unique<Bucket[]>(bucket_count)), words((countries + 63) / 64) {}
    ~Names() {
        for (size_t i = 0; i < bucket_count; ++i) {
            Bucket* bucket = &roots[i];
            while (bucket) {
                Bucket* next = bucket->next.load(std::memory_order_relaxed);
                if (bucket != &roots[i]) delete bucket;
                bucket = next;
            }
        }
    }
    bool add(std::string_view key, uint16_t country, std::pmr::memory_resource& arena) {
        size_t hash = std::hash<std::string_view>{}(key);
        Bucket* bucket = &roots[hash & (bucket_count - 1)];
        Entry* candidate = nullptr;
        for (;;) {
            for (auto& slot : bucket->slots) {
                Entry* found = slot.load(std::memory_order_acquire);
                if (!found) {
                    if (!candidate) {
                        void* storage = arena.allocate(sizeof(Entry), alignof(Entry));
                        candidate = std::construct_at(static_cast<Entry*>(storage), key, hash, words, arena);
                    }
                    Entry* expected = nullptr;
                    if (slot.compare_exchange_strong(expected, candidate,
                                                     std::memory_order_acq_rel, std::memory_order_acquire))
                        found = candidate;
                    else found = expected;
                }
                if (found->hash == hash && found->key == key) return mark(found, country);
            }
            // Slots never become empty. Advancing only after seeing a full
            // bucket makes insertion unique even when overflow creation races.
            Bucket* next = bucket->next.load(std::memory_order_acquire);
            if (!next) {
                auto extra = std::make_unique<Bucket>();
                Bucket* expected = nullptr;
                if (bucket->next.compare_exchange_strong(expected, extra.get(),
                                                         std::memory_order_acq_rel, std::memory_order_acquire))
                    next = extra.release();
                else next = expected;
            }
            bucket = next;
        }
    }
};

struct Worker {
    // One allocator per parser: published immutable storage survives every
    // lookup and is released in bulk only after all parser threads have joined.
    std::pmr::monotonic_buffer_resource arena;
    std::vector<Country> countries;
};

struct Threads {
    std::vector<std::thread> values;
    void join() { for (auto& t : values) if (t.joinable()) t.join(); }
    ~Threads() { join(); }
};

std::string money(int64_t value) {
    std::string sign = value < 0 ? "-" : "";
    uint64_t magnitude = value < 0 ? uint64_t(-(value + 1)) + 1 : uint64_t(value);
    auto fraction = magnitude % 100;
    return sign + std::to_string(magnitude / 100) + '.' + (fraction < 10 ? "0" : "")
           + std::to_string(fraction);
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
        if (!worker_count || worker_count > 12) throw std::runtime_error("workers must be 1..12");
        std::vector<Country> countries;
        countries.emplace_back("_UNMATCHED");
        std::unordered_map<std::string, uint16_t> codes;
        std::vector<uint16_t> regions(1, 0);
        {
            Input lookup(data_dir + "/regions.csv");
            Parser csv{lookup.data, lookup.data + lookup.size};
            while (csv.p < csv.end && *csv.p++ != '\n') {}
            while (csv.p < csv.end) {
                uint64_t id = csv.integer();
                ++csv.p;
                std::string code(csv.p, 3);
                csv.p += 3;
                csv.terminator();
                auto [it, inserted] = codes.try_emplace(code, static_cast<uint16_t>(countries.size()));
                if (inserted) countries.emplace_back(code);
                if (id >= regions.size()) regions.resize(id + 1, 0);
                regions[id] = it->second;
            }
        }
        Input records(data_dir + "/records.csv");
        const char* end = records.data + records.size;
        Parser header{records.data, end};
        header.bom();
        while (header.p < end && *header.p++ != '\n') {}
        std::vector<const char*> boundaries(worker_count + 1, end);
        boundaries[0] = header.p;
        const char* previous = header.p;
        bool quoted = false;
        for (size_t i = 1; i < worker_count; ++i) {
            const char* cut = header.p + (end - header.p) * i / worker_count;
            // Quote parity is valid even across escaped pairs: each pair flips
            // twice. It identifies true record LFs without relying on content.
            quoted ^= (std::count(previous, cut, '"') & 1) != 0;
            previous = cut;
            bool state = quoted;
            while (cut < end) {
                char byte = *cut++;
                if (byte == '"') state = !state;
                if (byte == '\n' && !state) break;
            }
            boundaries[i] = cut;
        }
        std::vector<Worker> partial(worker_count);
        for (auto& worker : partial) worker.countries = countries;
        Names names(countries.size());
        auto parse = [&](size_t i) {
            auto& worker = partial[i];
            Parser csv{boundaries[i], boundaries[i + 1]};
            std::string name;
        while (csv.p < csv.end) {
            csv.bom();  // this is a true record boundary, never an embedded LF
            uint64_t id = csv.integer();
            ++csv.p;
            auto name_view = csv.name(name);
            size_t name_bytes = name_view.size();
            ++csv.p;
            size_t note_bytes = csv.text(nullptr);
            ++csv.p;
            uint64_t region = csv.integer();
            ++csv.p;
            int64_t amount = csv.cents();
            csv.terminator();
            uint16_t country_id = region < regions.size() ? regions[region] : 0;
            auto& country = worker.countries[country_id];
            ++country.count;
            country.ids += id;
            country.sum += amount;
            country.min = std::min(country.min, amount);
            country.max = std::max(country.max, amount);
            country.name_bytes += name_bytes;
            country.note_bytes += note_bytes;
            country.distinct += names.add(name_view, country_id, worker.arena);
        }
        };
        Threads threads;
        for (size_t i = 1; i < worker_count; ++i) threads.values.emplace_back(parse, i);
        parse(0);
        threads.join();
        for (const auto& worker : partial) {
            for (size_t c = 0; c < countries.size(); ++c) {
                auto& target = countries[c];
                const auto& local = worker.countries[c];
                target.count += local.count;
                target.ids += local.ids;
                target.sum += local.sum;
                target.min = std::min(target.min, local.min);
                target.max = std::max(target.max, local.max);
                target.name_bytes += local.name_bytes;
                target.note_bytes += local.note_bytes;
                target.distinct += local.distinct;
            }
        }
        std::sort(countries.begin(), countries.end(), [](const Country& a, const Country& b) {
            return a.code < b.code;
        });
        std::ofstream out(out_dir + "/aggregates.txt", std::ios::binary);
        for (const auto& country : countries) {
            if (!country.count) continue;
            out << country.code << ' ' << country.count << ' ' << country.ids << ' '
                << money(country.sum) << ' ' << money(country.min) << ' ' << money(country.max)
                << ' ' << country.distinct << ' ' << country.name_bytes << ' '
                << country.note_bytes << '\n';
        }
        if (!out) throw std::runtime_error("output write failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
