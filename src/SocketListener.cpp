#include <server/SocketListener.hpp>

SocketListener::SocketListener(int max_events, int epoll_flags) : maxEvents_(max_events) {
  epollFd_ = epoll_create1(epoll_flags);
  if (epollFd_ < 0) {
    throw std::runtime_error(std::string("epoll_create failed: ") + strerror(errno));
  }
  events_.resize(maxEvents_);
}

SocketListener::~SocketListener() { close(epollFd_); }

void SocketListener::add_socket(int fd, Callback cb, uint32_t events) {
  makeNonBlocking(fd);

  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  auto it = callbacks_.find(fd);
  if (it != callbacks_.end()) {
    removeSocket(fd);
  }

  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
    throw std::runtime_error(std::string("epoll_ctl ADD failed: ") + strerror(errno));
  }
  callbacks_[fd] = std::move(cb);
}

void SocketListener::removeSocket(int fd) {
  epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
  callbacks_.erase(fd);
}

void SocketListener::run() {
  running_ = true;
  const int timeoutMs = 1000;
  while (running_) {
    const int n = epoll_wait(epollFd_, events_.data(), maxEvents_, timeoutMs);
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

void SocketListener::makeNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    throw std::runtime_error(std::string("fcntl GETFL failed: ") + strerror(errno));
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::runtime_error(std::string("fcntl SETFL failed: ") + strerror(errno));
}
