#include <gtest/gtest.h>
#include <server/HttpRequest.hpp>
#include <server/HttpResponse.hpp>

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

