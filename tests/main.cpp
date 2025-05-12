#include <gtest/gtest.h>
#include <server/WebServer.hpp>
#include <server/HttpRequest.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>

static WebServer::Parameters makeParams() {
  WebServer::Parameters p;
  p.host = "localhost";
  p.port = 0;
  return p;
}

static std::string extract_http_body(const std::string& raw_response) {
  size_t pos = raw_response.find("\r\n\r\n");
  if (pos == std::string::npos) {
    return "";
  }
  return raw_response.substr(pos + 4);
}

TEST(OnHttpTest, DispatchesGetHandler) {
  WebServer server(makeParams());
  bool called = false;
  server.get("/hello", [&](const HttpRequest& req) {
    called = true;
    return HttpResponse::Text("Hello", 200);
  });
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
  const char *req = "GET /hello HTTP/1.1\r\nHost: test\r\n\r\n";
  write(fds[0], req, strlen(req));
  server.on_http(fds[1]);
  char buf[256] = {0};
  ssize_t n = read(fds[0], buf, sizeof(buf) - 1);
  ASSERT_GT(n, 0);
  std::string response(buf);
  std::string body = extract_http_body(response);
  EXPECT_TRUE(called);
  EXPECT_EQ(body, "Hello");
  close(fds[0]);
  close(fds[1]);
}

TEST(OnHttpTest, DoesNotReactToAnotherRoutes) {
  WebServer server(makeParams());
  bool called = false;
  server.get("/how_are_u", [&](const HttpRequest& req) {
    called = true;
    return HttpResponse::Text("Hello", 200);
  });
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
  const char *req = "GET /hello HTTP/1.1\r\nHost: test\r\n\r\n";
  write(fds[0], req, strlen(req));
  server.on_http(fds[1]);
  char buf[256] = {0};
  ssize_t n = read(fds[0], buf, sizeof(buf) - 1);
  ASSERT_GT(n, 0);
  std::string response(buf);
  std::string body = extract_http_body(response);
  EXPECT_FALSE(called);
  EXPECT_EQ(body, "404 Not Found: /hello");
  close(fds[0]);
  close(fds[1]);
}

TEST(OnHttpTest, DispatchesPostHandler) {
  WebServer server(makeParams());
  bool called = false;
  server.post("/submit", [&](const HttpRequest& req) {
    called = true;
    return HttpResponse::Text("Posted", 200);
  });
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
  const char *req =
      "POST /submit HTTP/1.1\r\n"
      "Host: test\r\n"
      "Content-Length: 0\r\n"
      "\r\n";
  write(fds[0], req, strlen(req));
  server.on_http(fds[1]);
  char buf[256] = {0};
  ssize_t n = read(fds[0], buf, sizeof(buf) - 1);
  ASSERT_GT(n, 0);
  std::string response(buf);
  std::string body = extract_http_body(response);
  EXPECT_TRUE(called);
  EXPECT_EQ(body, "Posted");
  close(fds[0]);
  close(fds[1]);
}

TEST(RunStopTest, CanStartAndStopWithoutCrashing) {
  WebServer server(makeParams());
  EXPECT_NO_THROW(server.run());
  std::this_thread::sleep_for(std::chrono::seconds(5));
  EXPECT_NO_THROW(server.stop());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
