#include <gtest/gtest.h>
#include <WebServer.hpp>
#include <HttpRequest.hpp>
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


TEST(OnHttpTest, DispatchesGetHandler) {
  WebServer server(makeParams());
  bool called = false;

  server.get("/hello", [&](int client_fd, HttpRequest req) {
    called = true;
    const char* resp = "Hello";
    write(client_fd, resp, strlen(resp));
  });

  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  const char* req = "GET /hello HTTP/1.1\r\nHost: test\r\n\r\n";
  write(fds[0], req, strlen(req));

  server.on_http(fds[1]);

  char buf[64] = {0};
  ssize_t n = read(fds[0], buf, sizeof(buf)-1);
  buf[n] = '\0';

  EXPECT_TRUE(called);
  EXPECT_STREQ(buf, "Hello");

  close(fds[0]);
  close(fds[1]);
}

TEST(OnHttpTest, DispatchesPostHandler) {
  WebServer server(makeParams());
  bool called = false;

  server.post("/submit", [&](int client_fd, HttpRequest req) {
    called = true;
    write(client_fd, "Posted", 6);
  });

  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  const char* req =
      "POST /submit HTTP/1.1\r\n"
      "Host: test\r\n"
      "Content-Length: 0\r\n"
      "\r\n";
  write(fds[0], req, strlen(req));

  server.on_http(fds[1]);

  char buf[64] = {0};
  ssize_t n = read(fds[0], buf, sizeof(buf)-1);
  buf[n] = '\0';

  EXPECT_TRUE(called);
  EXPECT_STREQ(buf, "Posted");

  close(fds[0]);
  close(fds[1]);
}

TEST(RunStopTest, CanStartAndStopWithoutCrashing) {
  WebServer server(makeParams());
  EXPECT_NO_THROW(server.run());
  std::this_thread::sleep_for(std::chrono::seconds(5));
  EXPECT_NO_THROW(server.stop());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
