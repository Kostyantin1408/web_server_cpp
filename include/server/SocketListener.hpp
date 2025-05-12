#ifndef SOCKETLISTENER_HPP
#define SOCKETLISTENER_HPP

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class SocketListener {
public:
  struct Parameters {
    int max_events{16};
    int epoll_flags{0};
  };
  using Callback = std::function<void(int)>;

  explicit SocketListener(Parameters parameters);

  ~SocketListener();

  void add_socket(int fd, Callback cb, uint32_t events = EPOLLIN | EPOLLET);

  void removeSocket(int fd);

  void run();

  void stop();

private:
  void makeNonBlocking(int fd);

  Parameters parameters_;
  int epollFd_{-1};
  std::vector<epoll_event> events_;
  std::unordered_map<int, Callback> callbacks_;
  bool running_{false};
};

#endif // SOCKETLISTENER_HPP
