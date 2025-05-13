#include <gtest/gtest.h>
#include <HttpRequest.hpp>
#include <HttpResponse.hpp>

#include "encryption.hpp"

TEST(HttpRequestTest, ParsesQueryParams) {
    std::string raw =
            "GET /search?q=cpp&lang=en HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "\r\n";
    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_EQ(req.method, HttpRequest::HttpMethod::HTTP_GET);
    EXPECT_EQ(req.path, "/search");
    ASSERT_EQ(req.query_params.at("q"), "cpp");
    ASSERT_EQ(req.query_params.at("lang"), "en");
}

TEST(HttpRequestTest, DetectsWebSocketUpgrade) {
    std::string raw =
        "GET /ws HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: abc123\r\n"
        "\r\n";
    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_TRUE(req.is_websocket_upgrade());
    EXPECT_EQ(req.get_websocket_key(), "abc123");
}

TEST(HttpResponseTest, BuildsPlainTextResponse) {
    HttpResponse res = HttpResponse::Text("Hello", 200);
    std::string str = res.to_string();
    EXPECT_NE(str.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(str.find("Content-Type: text/plain"), std::string::npos);
    EXPECT_NE(str.find("Hello"), std::string::npos);
}

TEST(HttpResponseTest, NotFoundResponseIncludesMessage) {
    HttpResponse res = HttpResponse::NotFound("missing");
    EXPECT_EQ(res.status_code, 404);
    EXPECT_EQ(res.body, "missing");
}

TEST(EncryptionTest, GeneratesValidWebSocketAccept) {
    std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    std::string expected = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
    EXPECT_EQ(make_websocket_accept(key), expected);
}

TEST(HttpRequestTest, ParsesChunkedBody_Basic) {
    std::string raw =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "Hello, \r\n"
        "6\r\n"
        "world!\r\n"
        "0\r\n"
        "\r\n";

    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_EQ(req.body, "Hello, world!");
}

TEST(HttpRequestTest, ParsesChunkedBody_WithEmptyLines) {
    std::string raw =
        "POST /data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "abc\r\n\r\n"
        "4\r\n"
        "1234\r\n"
        "0\r\n"
        "\r\n";

    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_EQ(req.body, "abc\r\n1234");
}

TEST(HttpRequestTest, ParsesChunkedBody_ImmediateZero) {
    std::string raw =
        "POST /empty HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n"
        "\r\n";

    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_EQ(req.body, "");
}

TEST(HttpRequestTest, ParsesChunkedBody_HexUpperCase) {
    std::string raw =
        "POST /hex HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "A\r\n"
        "0123456789\r\n"
        "0\r\n"
        "\r\n";

    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_EQ(req.body, "0123456789");
}

TEST(HttpRequestTest, ParsesChunkedBody_WithTrailingNewlines) {
    std::string raw =
        "POST /withnl HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "6\r\n"
        "hello\n\r\n"
        "1\r\n"
        "!\r\n"
        "0\r\n"
        "\r\n";

    HttpRequest req = HttpRequest::parse_http_request(raw);
    EXPECT_EQ(req.body, "hello\n!");
}

TEST(HttpRequestTest, FailsOnMalformedChunked) {
    std::string raw =
        "POST /fail HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "ZZ\r\n"
        "Oops\r\n"
        "0\r\n"
        "\r\n";

    EXPECT_NO_THROW({
        HttpRequest req = HttpRequest::parse_http_request(raw);
        EXPECT_TRUE(req.body.empty());
    });
}
