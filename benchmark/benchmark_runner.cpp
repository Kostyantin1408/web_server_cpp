#include <httplib.h>
#include <chrono>
#include <iostream>
#include "custom_server.hpp"
#include "httplib_server.hpp"
#include "utils.hpp"

struct BenchmarkResult {
    std::string name;
    long long total_us;
    double rps;
};


BenchmarkResult  benchmark(const std::string &name, std::function<void()> launch_server, int port = 8080) {
    launch_server();

    httplib::Client cli("127.0.0.1", port);
    constexpr int N = 10000;

    auto start = get_current_time_fenced();

    for (int i = 0; i < N; ++i) {
        auto res = cli.Get("/hello");
        if (!res || res->status != 200 || res->body != "OK") {
            std::cerr << "Invalid response at " << i << "\n";
            break;
        }
    }

    auto end = get_current_time_fenced();
    auto us = to_us(end - start);
    double seconds = us / 1'000'000.0;
    double rps = N / seconds;

    return {name, us, rps};
}

void print_results_table(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n| Server Name        | Time (ms) | Requests/sec |\n";
    std::cout << "|--------------------|-----------|--------------|\n";
    for (const auto& r : results) {
        std::cout << "| " << std::left << std::setw(19) << r.name
                  << "| " << std::right << std::setw(9) << r.total_us / 1000
                  << " | " << std::setw(12) << static_cast<int>(r.rps) << " |\n";
    }
}

int main() {
    std::cout << "--- Benchmark Start ---\n";

    std::vector<BenchmarkResult> results;

    results.push_back(benchmark("Custom WebServer", [] {
        static auto server = launch_custom_server(8082);
    }, 8082));

    results.push_back(benchmark("cpp-httplib", [] {
        static auto server = launch_httplib_server(8081);
    }, 8081));

    print_results_table(results);
}
