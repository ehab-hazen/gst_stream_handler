#pragma once

#include <deque>
#include <mutex>
#include <optional>

template <typename T> class SharedQueue {

  using WLock = std::lock_guard<std::mutex>;

public:
  void push(const T &val) {
    WLock lk(mtx_);
    q_.push_back(val);
  }

  void push(T &&val) {
    WLock lk(mtx_);
    q_.push_back(std::move(val));
  }

  std::optional<T> pop() {
      WLock lk(mtx_);

      if (q_.empty()) {
          return std::nullopt;
      }

      T val = q_.front();
      q_.pop_front();
      return std::make_optional<T>(val);
  }

private:
  std::deque<T> q_;
  std::mutex mtx_;
};
