#pragma once

#include <chrono>  // NOLINT

namespace cinn {

class Timer {
  std::chrono::system_clock::time_point start_, end_;

 public:
  using time_t = std::chrono::high_resolution_clock;
  using ms = std::chrono::milliseconds;
  using time_point_t = std::chrono::time_point<ms>;

  void Start() { start_ = time_t::now(); }
  void Stop() { end_ = time_t::now(); }
  long duration() const { return std::chrono::duration_cast<ms>(end_ - start_).count(); }
};

}  // namespace cinn
