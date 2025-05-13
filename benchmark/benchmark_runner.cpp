#include <httplib.h>
#include <chrono>
#include <iostream>
#include "custom_server.hpp"
#include "httplib_server.hpp"
#include "utils.hpp"

static std::string generate_large_body(size_t size_in_bytes) {
    return std::string(size_in_bytes, 'x');
}

static std::string generate_large_request_content_length(size_t size_in_bytes) {
    std::string body = generate_large_body(size_in_bytes);
    std::ostringstream oss;
    oss << "POST /upload HTTP/1.1\r\n"
        << "Host: localhost\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n"
        << body;
    return oss.str();
}

static std::string generate_large_request_chunked(size_t size_in_bytes, size_t chunk_size = 4096) {
    std::ostringstream oss;
    oss << "POST /upload HTTP/1.1\r\n"
        << "Host: localhost\r\n"
        << "Transfer-Encoding: chunked\r\n"
        << "\r\n";

    size_t remaining = size_in_bytes;
    while (remaining > 0) {
        size_t current = std::min(remaining, chunk_size);
        oss << std::hex << current << "\r\n"
            << std::string(current, 'x') << "\r\n";
        remaining -= current;
    }
    oss << "0\r\n\r\n";
    return oss.str();
}


void benchmark(const std::string &name, std::function<void()> launch_server, int port = 8080) {
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

    std::cout << name << ": " << N << " requests in " << us << " µs\n";
    std::cout << "RPS: " << static_cast<int>(rps) << " requests/second\n";
}

int main() {
    std::cout << "--- Benchmark Start ---\n";
    benchmark("Custom WebServer", [] {
        static auto server = launch_custom_server(8080);
    }, 8080);

    benchmark("cpp-httplib", [] {
        static auto server = launch_httplib_server(8081);
    }, 8081);
}
