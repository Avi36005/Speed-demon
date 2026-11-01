#include <algorithm>
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
    std::vector<uint64_t> names;
    uint64_t distinct = 0;
    explicit Country(std::string key) : code(std::move(key)) {}
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
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (i + 1 < argc && arg == "--data") data_dir = argv[++i];
            else if (i + 1 < argc && arg == "--out") out_dir = argv[++i];
            else throw std::runtime_error("usage: solution [--data DIR] [--out DIR]");
        }
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
        Parser csv{records.data, records.data + records.size};
        csv.bom();
        while (csv.p < csv.end && *csv.p++ != '\n') {}  // header has no quotes
        std::string name;
        std::unordered_map<std::string, uint32_t> names;
        names.reserve(131072);
        while (csv.p < csv.end) {
            csv.bom();  // this is a true record boundary, never an embedded LF
            uint64_t id = csv.integer();
            ++csv.p;
            size_t name_bytes = csv.text(&name);
            ++csv.p;
            size_t note_bytes = csv.text(nullptr);
            ++csv.p;
            uint64_t region = csv.integer();
            ++csv.p;
            int64_t amount = csv.cents();
            csv.terminator();
            auto& country = countries[region < regions.size() ? regions[region] : 0];
            ++country.count;
            country.ids += id;
            country.sum += amount;
            country.min = std::min(country.min, amount);
            country.max = std::max(country.max, amount);
            country.name_bytes += name_bytes;
            country.note_bytes += note_bytes;
            // Interning compares decoded bytes exactly. Membership bits share
            // those IDs across countries; they do not stand in for name hashes.
            auto [it, inserted] = names.try_emplace(name, static_cast<uint32_t>(names.size()));
            uint32_t name_id = it->second;
            size_t word = name_id / 64;
            if (word >= country.names.size()) country.names.resize(word + 1, 0);
            uint64_t mask = uint64_t{1} << (name_id % 64);
            if (!(country.names[word] & mask)) {
                country.names[word] |= mask;
                ++country.distinct;
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
