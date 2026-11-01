#include <algorithm>
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
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

using Table = std::unordered_map<std::string_view, std::array<Group, 72>>;
struct Worker {
    Table endpoints;
    std::array<std::string_view, 72> hours{};
};

// Apple libc++ lacks jthread; explicit ownership also joins on exceptions.
struct ThreadGroup {
    std::vector<std::thread> values;
    void join() {
        for (auto& thread : values) if (thread.joinable()) thread.join();
    }
    ~ThreadGroup() { join(); }
};

void parse(const char* p, const char* end, Worker& worker) {
    worker.endpoints.reserve(2048);
    while (p < end) {
        // The frozen grammar fixes UTC timestamps and this 72-hour span.
        int day = p[6] == '0' ? (p[8] - '0') * 10 + p[9] - '0' - 30 : p[9] - '0' + 1;
        int hour = day * 24 + (p[11] - '0') * 10 + p[12] - '0' - 18;
        worker.hours[hour] = std::string_view(p, 13);
        p += 28;
        while (*p != ' ') ++p;
        const char* start = ++p;
        while (*p != ' ') ++p;
        std::string_view endpoint(start, p - start);
        bool error = p[1] == '5';
        p += 5;
        uint32_t latency = 0;
        while (*p != '\n') latency = latency * 10 + static_cast<unsigned>(*p++ - '0');
        ++p;
        auto& group = worker.endpoints[endpoint][hour];
        group.values.push_back(latency);
        group.errors += error;
    }
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
        Input input(data_dir + "/requests.log");
        std::vector<size_t> boundaries(worker_count + 1, input.size);
        boundaries[0] = 0;
        for (size_t i = 1; i < worker_count; ++i) {
            size_t offset = input.size * i / worker_count;
            while (offset > 0 && offset < input.size && input.data[offset - 1] != '\n') ++offset;
            boundaries[i] = offset;
        }
        std::vector<Worker> partial(worker_count);
        ThreadGroup threads;
        for (size_t i = 1; i < worker_count; ++i)
            threads.values.emplace_back([&, i] {
                parse(input.data + boundaries[i], input.data + boundaries[i + 1], partial[i]);
            });
        parse(input.data, input.data + boundaries[1], partial[0]);
        threads.join();  // every worker finishes before any merging
        Table endpoints = std::move(partial[0].endpoints);
        std::array<std::string_view, 72> hours{};
        for (size_t i = 0; i < worker_count; ++i) {
            for (size_t h = 0; h < 72; ++h)
                if (!partial[i].hours[h].empty()) hours[h] = partial[i].hours[h];
            if (i == 0) continue;
            for (auto& [endpoint, groups] : partial[i].endpoints) {
                auto& target = endpoints[endpoint];
                for (size_t h = 0; h < 72; ++h) {
                    auto& to = target[h];
                    auto& from = groups[h];
                    to.errors += from.errors;
                    if (to.values.empty()) to.values = std::move(from.values);
                    else to.values.insert(to.values.end(), from.values.begin(), from.values.end());
                }
            }
        }
        partial.clear();
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
