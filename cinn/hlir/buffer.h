#pragma once
#include <stddef.h>
#include <string>
#include "cinn/utils/name_generator.h"

namespace cinn {
namespace hlir {

/**
 * Buffer represents the underlying memory buffer that supports the analysis of the memory of the HLIR.
 */
class Buffer {
 public:
  Buffer() : name_(NameGenerator::Global().NewBuffer()) {}

  // TODO(Superjomn) Add memory management.
  void Resize(size_t size) { size_ = size; }

  size_t size() const { return size_; }

 private:
  size_t size_{};
  std::string name_;
};

}  // namespace hlir
}  // namespace cinn
