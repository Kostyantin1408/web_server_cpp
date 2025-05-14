#include <httplib.h>
#include <chrono>
#include <iostream>
#include "custom_server.hpp"
#include "httplib_server.hpp"
#include "crow_server.hpp"
#include "utils.hpp"
#include <numeric>
#include <cmath>


struct BenchmarkResult {
    std::string name;
    double avg_us;
    double min_us;
    double std_dev_us;
    double avg_rps;
};

BenchmarkResult benchmark(const std::string &name, const std::function<void()>& launch_server,
                          const std::function<void()>& stop_server, int port) {
    constexpr int num_iterations = 200;
    constexpr int N = 1000;

    std::cout << "--- Benchmark for " << name << " started (" << num_iterations << " iterations) ---\n";

    std::vector<long long> durations;

    launch_server();
    for (int i = 0; i < num_iterations; ++i) {
        httplib::Client cli("127.0.0.1", port);

        auto start = get_current_time_fenced();

        int j = 0;
        for (; j < N; ++j) {
            auto res = cli.Get("/hello");
            if (!res || res->status != 200 || res->body != "OK")
                break;
        }
        if (j != N)
            continue;

        auto end = get_current_time_fenced();
        auto duration = to_ms(end - start);
        if (duration < 0)
            continue;
        durations.push_back(duration);
    }
    stop_server();

    double sum = std::accumulate(durations.begin(), durations.end(), 0.0);
    double avg = sum / static_cast<double>(durations.size());
    double min = static_cast<double>(*std::ranges::min_element(durations));

    double variance = 0.0;
    for (auto d: durations) {
        variance += (static_cast<double>(d) - avg) * (static_cast<double>(d) - avg);
    }
    double stddev = std::sqrt(variance / static_cast<double>(durations.size()));
    double avg_rps = 1000.0 * N / avg;

    return {name, avg, min, stddev, avg_rps};
}


void print_results_table(const std::vector<BenchmarkResult> &results) {
    std::cout << "\n| Server Name        | Avg (us) | Min (us) | StdDev (us) | Avg RPS   |\n";
    std::cout << "|--------------------|----------|----------|-------------|-----------|\n";
    for (const auto &r: results) {
        std::cout << "| " << std::left << std::setw(19) << r.name
                << "| " << std::right << std::setw(8) << static_cast<int>(r.avg_us)
                << " | " << std::setw(8) << static_cast<int>(r.min_us)
                << " | " << std::setw(11) << static_cast<int>(r.std_dev_us)
                << " | " << std::setw(9) << static_cast<int>(r.avg_rps)
                << " |\n";
    }
}

int main() {
    std::vector<BenchmarkResult> results;

    std::unique_ptr<WebServer> custom_server_instance;
    results.push_back(benchmark(
        "Custom WebServer",
        [&]() { custom_server_instance = launch_custom_server(8081); },
        [&]() {
            custom_server_instance->request_stop();
            custom_server_instance->wait_for_exit();
        },
        8081
    ));

    std::unique_ptr<httplib::Server> httplib_server_instance;
    results.push_back(benchmark(
        "cpp-httplib",
        [&]() { httplib_server_instance = launch_httplib_server(8082); },
        [&]() { httplib_server_instance->stop(); },
        8082
    ));

    std::unique_ptr<CrowServerWrapper> crow_server_instance;
    results.push_back(benchmark(
        "Crow",
        [&]() { crow_server_instance = launch_crow_server(8084); },
        [&]() { crow_server_instance->stop(); },
        8084
    ));

    print_results_table(results);

    return 0;
}
