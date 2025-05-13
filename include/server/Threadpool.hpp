#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <thread>

class Threadpool {
public:
  explicit Threadpool(size_t num_threads);
  ~Threadpool();

  void submit(std::function<void()> task);

  void request_stop();

  void wait_for_exit();

private:
  void worker_loop(const std::stop_token &stoken);

  std::vector<std::jthread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mtx_;
  std::condition_variable_any cv_;
  bool shutdown_{false};
};

#endif // THREADPOOL_HPP