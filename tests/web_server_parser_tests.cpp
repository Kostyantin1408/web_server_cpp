#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include "HttpRequest.hpp"
#include "WebServer.hpp"

const std::string chunked_request =
    "POST /test HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Transfer-Encoding: chunked\r\n"
    "\r\n"
    "7\r\nHello, \r\n"
    "6\r\nworld!\r\n"
    "0\r\n\r\n";

const std::string length_request =
    "POST /test HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: 13\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Hello, world!";

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

TEST(BenchmarkHttpRequest, ParseChunked10000) {
    constexpr int N = 10000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        HttpRequest req = HttpRequest::parse_http_request(chunked_request);
        ASSERT_EQ(req.body, "Hello, world!");
    }
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[Chunked] Parsed " << N << " requests in " << ms << " ms\n";
}

TEST(BenchmarkHttpRequest, ParseContentLength10000) {
    constexpr int N = 10000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        HttpRequest req = HttpRequest::parse_http_request(length_request);
        ASSERT_EQ(req.body, "Hello, world!");
    }
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[Content-Length] Parsed " << N << " requests in " << ms << " ms\n";
}

TEST(BenchmarkWebServer, OnHttpSimpleRequest) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    WebServer server({.host="localhost", .port=0});
    server.post("/test", [](const HttpRequest& req) {
        return HttpResponse::Text("ACK");
    });

    write(fds[0], length_request.c_str(), length_request.size());

    auto start = std::chrono::steady_clock::now();
    server.on_http(fds[1]);
    auto end = std::chrono::steady_clock::now();

    char buf[256] = {};
    read(fds[0], buf, sizeof(buf));
    std::cout << "[on_http] Response: " << buf << std::endl;

    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "[WebServer] on_http handled in " << ms << " us\n";

    close(fds[0]);
    close(fds[1]);
}

TEST(BenchmarkHttpRequest, ParseContentLength1MB) {
    auto request = generate_large_request_content_length(1 << 20);
    auto start = std::chrono::steady_clock::now();
    HttpRequest req = HttpRequest::parse_http_request(request);
    auto end = std::chrono::steady_clock::now();

    ASSERT_EQ(req.body.size(), 1 << 20);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[1MB Content-Length] Parsed in " << ms << " ms\n";
}

TEST(BenchmarkHttpRequest, ParseChunked1MB) {
    auto request = generate_large_request_chunked(1 << 20);
    auto start = std::chrono::steady_clock::now();
    HttpRequest req = HttpRequest::parse_http_request(request);
    auto end = std::chrono::steady_clock::now();

    ASSERT_EQ(req.body.size(), 1 << 20);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[1MB Chunked] Parsed in " << ms << " ms\n";
}

TEST(BenchmarkHttpRequest, ParseChunked10MB) {
    auto request = generate_large_request_chunked(10 << 20);
    auto start = std::chrono::steady_clock::now();
    HttpRequest req = HttpRequest::parse_http_request(request);
    auto end = std::chrono::steady_clock::now();

    ASSERT_EQ(req.body.size(), 10 << 20);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[10MB Chunked] Parsed in " << ms << " ms\n";
}

TEST(BenchmarkHttpRequest, ParseContentLength1MB_1000x) {
    const int N = 1000;
    auto request = generate_large_request_content_length(1 << 20);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        HttpRequest req = HttpRequest::parse_http_request(request);
        ASSERT_EQ(req.body.size(), 1 << 20);
    }
    auto end = std::chrono::steady_clock::now();

    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double avg_ms = total_ms / static_cast<double>(N);

    std::cout << "[1MB Content-Length × " << N << "] Total: " << total_ms << " ms, Avg: "
              << avg_ms << " ms/request\n";
}


TEST(BenchmarkHttpRequest, ParseChunked1MB_1000x) {
    const int N = 1000;
    auto request = generate_large_request_chunked(1 << 20);
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        HttpRequest req = HttpRequest::parse_http_request(request);
        ASSERT_EQ(req.body.size(), 1 << 20);
    }
    auto end = std::chrono::steady_clock::now();

    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double avg_ms = total_ms / static_cast<double>(N);

    std::cout << "[1MB Chunked × " << N << "] Total: " << total_ms << " ms, Avg: "
              << avg_ms << " ms/request\n";
}

