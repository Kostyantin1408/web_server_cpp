#include <server/SocketListener.hpp>

SocketListener::SocketListener(Parameters parameters, Callback callback) : parameters_(std::move(parameters)), callback_(std::move(callback)) {
  epoll_fd_ = epoll_create1(parameters_.epoll_flags);
  if (epoll_fd_ < 0) {
    throw std::runtime_error(std::string("epoll_create failed: ") + strerror(errno));
  }
  events_.resize(parameters_.max_events);
}

SocketListener::~SocketListener() { close(epoll_fd_); }

void SocketListener::add_socket(const int fd, const uint32_t events) {
  make_non_blocking(fd);

  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
    throw std::runtime_error(std::string("epoll_ctl ADD failed: ") + strerror(errno));
  }
}

void SocketListener::remove_socket(const int fd) const { epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr); }

void SocketListener::run() {
  running_ = true;
  while (running_) {
    const int n = epoll_wait(epoll_fd_, events_.data(), parameters_.max_events, parameters_.epoll_timeout);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      throw std::runtime_error(std::string("epoll_wait failed: ") + strerror(errno));
    }
    if (n == 0) {
      continue;
    }
    for (int i = 0; i < n; ++i) {
      const int fd = events_[i].data.fd;
      callback_(fd);
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
