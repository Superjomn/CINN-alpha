#pragma once
#include <stddef.h>

namespace cinn {
namespace model_loader {

/**
 * Buffer on host.
 */
class HostBuffer {
 public:
  HostBuffer() = default;
  explicit HostBuffer(size_t size) : size_(size) {}

  void Resize(size_t size) {
    if (space_ < size) {
      Free();
      data_ = new char[size];
      space_ = size;
    }
  }

  void Free() {
    if (space_ > 0) {
      Free();
      space_ = 0;
    }
  }

  void* data() const { return data_; }
  size_t space() const { return space_; }

 private:
  void* data_{};
  size_t size_{};
  size_t space_{};
};

}  // namespace model_loader
}  // namespace cinn
