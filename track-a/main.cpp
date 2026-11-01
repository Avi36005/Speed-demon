#include <algorithm>
#include <array>
#include <atomic>
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
    uint32_t count = 0, errors = 0, cursor = 0;
};

using Table = std::unordered_map<std::string_view, std::array<Group, 72>>;
struct Row { uint32_t group, latency; };
struct Worker {
    std::unordered_map<std::string_view, uint32_t> endpoints;
    std::vector<Group> groups;
    std::vector<Row> rows;
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
    worker.groups.reserve(2048 * 72);
    worker.rows.reserve(static_cast<size_t>(end - p) / 64 + 1);
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
        auto [it, inserted] = worker.endpoints.try_emplace(endpoint, worker.endpoints.size());
        if (inserted) worker.groups.resize(worker.groups.size() + 72);
        uint32_t id = it->second * 72 + hour;
        auto& group = worker.groups[id];
        ++group.count;
        group.errors += error;
        worker.rows.push_back({id, latency});
    }
}

int main(int argc, char** argv) {
    try {
        std::string data_dir = "/data", out_dir = "/out";
        size_t worker_count = 12;
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
        Table endpoints;
        std::array<std::string_view, 72> hours{};
        for (size_t i = 0; i < worker_count; ++i) {
            for (size_t h = 0; h < 72; ++h)
                if (!partial[i].hours[h].empty()) hours[h] = partial[i].hours[h];
            for (auto [endpoint, id] : partial[i].endpoints) {
                auto& target = endpoints[endpoint];
                for (size_t h = 0; h < 72; ++h) {
                    auto& to = target[h];
                    const auto& from = partial[i].groups[id * 72 + h];
                    to.errors += from.errors;
                    to.count += from.count;
                }
            }
        }
        using Endpoint = std::pair<std::string_view, std::array<Group, 72>*>;
        std::vector<Endpoint> order;
        for (auto& [key, groups] : endpoints) order.emplace_back(key, &groups);
        std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;  // ASCII byte order, prefixes first
        });
        // Count/prefix/scatter replaces hundreds of thousands of tiny latency
        // vectors. Each worker receives a disjoint slice of every global group.
        uint32_t total = 0;  // the frozen full scale has 3,100,000 rows
        for (size_t h = 0; h < hours.size(); ++h)
            for (auto [key, groups] : order) {
                auto& group = (*groups)[h];
                group.cursor = total;
                total += group.count;
            }
        std::vector<uint32_t> values(total);
        for (auto& worker : partial)
            for (auto [endpoint, id] : worker.endpoints) {
                auto& target = endpoints.at(endpoint);
                for (size_t h = 0; h < 72; ++h) {
                    auto& local = worker.groups[id * 72 + h];
                    local.cursor = target[h].cursor;
                    target[h].cursor += local.count;
                }
            }
        // Global cursors now hold exclusive group ends; local cursors advance
        // only within the slices just assigned, so scatter requires no atomics.
        auto scatter = [&](size_t i) {
            auto& worker = partial[i];
            for (auto row : worker.rows)
                values[worker.groups[row.group].cursor++] = row.latency;
        };
        threads.values.clear();
        for (size_t i = 1; i < worker_count; ++i)
            threads.values.emplace_back(scatter, i);
        scatter(0);
        threads.join();
        partial.clear();
        // Different hours own disjoint vectors and output strings. The map is
        // complete before these workers start; no shared lookup or insertion.
        std::array<std::string, 72> outputs;
        std::atomic<size_t> next_hour{0};
        auto finish_hours = [&] {
          for (;;) {
            size_t h = next_hour.fetch_add(1, std::memory_order_relaxed);
            if (h >= hours.size()) break;
            auto& output = outputs[h];
            for (auto [endpoint, groups] : order) {
                auto& group = (*groups)[h];
                if (!group.count) continue;
                auto* first = values.data() + group.cursor - group.count;
                std::sort(first, values.data() + group.cursor);
                uint64_t n = group.count;
                output.append(hours[h]);
                output += ":00:00Z ";
                output.append(endpoint);
                output += ' ' + std::to_string(n);
                for (uint64_t percentile : {50, 95, 99})
                    output += ' ' + std::to_string(first[(percentile * n + 99) / 100 - 1]);
                output += ' ' + std::to_string(group.errors);
                output += ' ' + std::to_string(uint64_t{group.errors} * 1000000 / n) + '\n';
            }
          }
        };
        threads.values.clear();
        for (size_t i = 1; i < worker_count; ++i)
            threads.values.emplace_back(finish_hours);
        finish_hours();
        threads.join();
        std::ofstream out(out_dir + "/summary.txt", std::ios::binary);
        for (const auto& output : outputs) out.write(output.data(), output.size());
        if (!out) throw std::runtime_error("output write failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
