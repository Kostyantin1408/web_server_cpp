#include <server/SocketListener.hpp>

SocketListener::SocketListener(Parameters parameters) : parameters_(parameters) {
  epoll_fd_ = epoll_create1(parameters_.epoll_flags);
  if (epoll_fd_ < 0) {
    throw std::runtime_error(std::string("epoll_create failed: ") + strerror(errno));
  }
  events_.resize(parameters_.max_events);
}

SocketListener::~SocketListener() { close(epoll_fd_); }

void SocketListener::add_socket(const int fd, Callback cb, const uint32_t events) {
  make_non_blocking(fd);

  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  auto it = callbacks_.find(fd);
  if (it != callbacks_.end()) {
    remove_socket(fd);
  }

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
    throw std::runtime_error(std::string("epoll_ctl ADD failed: ") + strerror(errno));
  }
  callbacks_[fd] = std::move(cb);
}

void SocketListener::remove_socket(int fd) {
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
  callbacks_.erase(fd);
}

void SocketListener::run() {
  running_ = true;
  const int timeoutMs = 1000;
  while (running_) {
    const int n = epoll_wait(epoll_fd_, events_.data(), parameters_.max_events, timeoutMs);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      throw std::runtime_error(std::string("epoll_wait failed: ") + strerror(errno));
    }
    if (n == 0) {
      continue;
    }
    for (int i = 0; i < n; ++i) {
      int fd = events_[i].data.fd;
      auto it = callbacks_.find(fd);
      if (it != callbacks_.end()) {
        it->second(fd);
      }
    }
  }
}

void SocketListener::stop() { running_ = false; }

void SocketListener::make_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    throw std::runtime_error(std::string("fcntl GETFL failed: ") + strerror(errno));
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::runtime_error(std::string("fcntl SETFL failed: ") + strerror(errno));
}
