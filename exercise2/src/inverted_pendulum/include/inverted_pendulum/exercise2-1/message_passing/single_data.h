#ifndef INVERTED_PENDULUM_SINGLE_DATA_H_
#define INVERTED_PENDULUM_SINGLE_DATA_H_

#include <atomic>
#include <mutex>

struct SingleData {
  static_assert(std::atomic<double>::is_always_lock_free);

  void Set(const double value) {
    std::scoped_lock lock(mutex_);
    value_ = value;
  }

  double Get() {
    std::scoped_lock lock(mutex_);
    return value_;
  }

 private:
  std::mutex mutex_;
  double     value_;
};

#endif
