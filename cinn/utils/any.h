#pragma once

#include <glog/logging.h>
#include <functional>
#include <set>

namespace cinn {

class Any {
 public:
  template <typename T>
  void set(const T& v) {
    set<T>();
    *get_mutable<T>() = v;
  }

  template <typename T>
  void set() {
    if (type_ != kInvalidType) {
      CHECK(type_ == typeid(T).hash_code());
    } else {
      type_ = typeid(T).hash_code();
      deleter_ = [&] { delete static_cast<T*>(data_); };
    }
    data_ = new T;
  }

  template <typename T>
  const T& get() const {
    CHECK(data_);
    CHECK(type_ == typeid(T).hash_code());
    return *static_cast<T*>(data_);
  }
  template <typename T>
  T* get_mutable() {
    CHECK(data_);
    CHECK(type_ == typeid(T).hash_code());
    return static_cast<T*>(data_);
  }

  bool valid() const { return data_; }

  // ~Any() {
  //    if (valid()) {
  //      deleter_();
  //    }
  //  }

 private:
  static size_t kInvalidType;
  size_t type_{kInvalidType};
  void* data_{nullptr};
  std::function<void()> deleter_;
};

}  // namespace cinn
