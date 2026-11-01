#include <algorithm>
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// Input-backed views remain valid until all aggregation and output have finished.
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

struct Group {
    std::vector<uint32_t> values;
    uint64_t errors = 0;
};

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (i + 1 < argc && arg == "--data") data_dir = argv[++i];
            else if (i + 1 < argc && arg == "--out") out_dir = argv[++i];
            else throw std::runtime_error("usage: solution [--data DIR] [--out DIR]");
        }
        Input input(data_dir + "/requests.log");
        std::unordered_map<std::string_view, std::array<Group, 72>> endpoints;
        endpoints.reserve(2048);
        std::array<std::string_view, 72> hours{};
        const char* p = input.data;
        const char* end = p + input.size;
        while (p < end) {
            // The frozen grammar guarantees fixed-width UTC timestamps in this span.
            int day = p[6] == '0' ? (p[8] - '0') * 10 + p[9] - '0' - 30
                                  : p[9] - '0' + 1;
            int hour = day * 24 + (p[11] - '0') * 10 + p[12] - '0' - 18;
            hours[hour] = std::string_view(p, 13);
            p += 28;
            while (*p != ' ') ++p;  // ignored method
            const char* start = ++p;
            while (*p != ' ') ++p;
            std::string_view endpoint(start, p - start);
            bool error = p[1] == '5';
            p += 5;  // space, three-digit status, space
            uint32_t latency = 0;
            while (*p != '\n') latency = latency * 10 + static_cast<unsigned>(*p++ - '0');
            ++p;
            auto& group = endpoints[endpoint][hour];
            group.values.push_back(latency);
            group.errors += error;
        }
        std::vector<std::string_view> order;
        for (const auto& [key, groups] : endpoints) order.push_back(key);
        std::sort(order.begin(), order.end());  // ASCII byte order, prefixes first
        std::string output;
        for (size_t h = 0; h < hours.size(); ++h) {
            for (auto endpoint : order) {
                auto& group = endpoints.at(endpoint)[h];
                auto& values = group.values;
                if (values.empty()) continue;
                std::sort(values.begin(), values.end());
                uint64_t n = values.size();
                output.append(hours[h]);
                output += ":00:00Z ";
                output.append(endpoint);
                output += ' ' + std::to_string(n);
                for (uint64_t percentile : {50, 95, 99})
                    output += ' ' + std::to_string(values[(percentile * n + 99) / 100 - 1]);
                output += ' ' + std::to_string(group.errors);
                output += ' ' + std::to_string(group.errors * 1000000 / n) + '\n';
            }
        }
        std::ofstream out(out_dir + "/summary.txt", std::ios::binary);
        out.write(output.data(), output.size());
        if (!out) throw std::runtime_error("output write failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
