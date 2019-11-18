#pragma once
#include <stddef.h>
#include <set>
#include <string>
#include "cinn/type.h"
#include "cinn/utils/any.h"
#include "cinn/utils/name_generator.h"

namespace cinn {
namespace hlir {

/**
 * Buffer represents the underlying memory buffer that supports the analysis of the memory of the HLIR.
 */
class Buffer {
 public:
  Buffer() = default;
  Buffer(const std::string& name, primitive_t ptype) : name_(name) {
    // CHECK(!names_.count(name)) << "duplicate buffer name: " << name;
    // names_.insert(name);
  }

  void set_name(const std::string& name) { name_ = name; }
  const std::string& name() const { return name_; }

  // TODO(Superjomn) Add memory management.
  void Resize(size_t size) { size_ = size; }

  size_t size() const { return size_; }

  template <typename T>
  void SetData(const T* x) {
    CHECK_GT(size(), 0UL);
    std::vector<T> data(x, x + size_ / sizeof(T));
    data_.set(data);
  }

  bool has_data() { return data_.valid(); }

  template <typename T>
  const T& data() {
    CHECK(has_data());
    return data_.get<T>();
  }
  const Any& data() const { return data_; }

 private:
  size_t size_{};
  std::string name_;

  Any data_;
};

}  // namespace hlir
}  // namespace cinn
