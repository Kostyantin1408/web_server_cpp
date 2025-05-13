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
  using Callback = std::function<void(int)>;
  struct Parameters {
    int max_events{16};
    int epoll_flags{0};
    int epoll_timeout{1000};
  };

  explicit SocketListener(Parameters parameters, Callback callback);

  ~SocketListener();

  void add_socket(int fd, uint32_t events = EPOLLIN | EPOLLET);

  void remove_socket(int fd) const;

  void run();

  void stop();

private:
  void make_non_blocking(int fd);

  Parameters parameters_;
  int epoll_fd_{-1};
  std::vector<epoll_event> events_;
  Callback callback_;
  bool running_{false};
};

#endif // SOCKETLISTENER_HPP
