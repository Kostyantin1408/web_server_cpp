#include <iostream>
#include <server/Threadpool.hpp>

Threadpool::Threadpool(const size_t num_threads) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back([this](const std::stop_token &stoken) { worker_loop(stoken); });
  }
}

Threadpool::~Threadpool() {
  {
    std::scoped_lock lock(mtx_);
    shutdown_ = true;
  }
  cv_.notify_all();
}

void Threadpool::submit(std::function<void()> task) {
  {
    std::scoped_lock lock(mtx_);
    if (shutdown_) {
      std::cerr << "Threadpool::submit() called on shutdown" << std::endl;
      return;
    }
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

void Threadpool::request_stop() {
  {
    std::scoped_lock lock(mtx_);
    shutdown_ = true;
  }
  cv_.notify_all();
  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.request_stop();
    }
  }
}


void Threadpool::wait_for_exit() {
  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void Threadpool::worker_loop(const std::stop_token &stoken) {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock lock(mtx_);
      cv_.wait(lock, stoken, [this] { return shutdown_ || !tasks_.empty(); });

      if (shutdown_ && tasks_.empty()) {
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }
    try {
      if (task) {
        task();
      }
    } catch (const std::exception &e) {
      std::cerr << "Threadpool task threw exception: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Threadpool task threw unknown exception" << std::endl;
    }
  }
}