#pragma once
#include <algorithm>
#include <atomic>
#include <utility>

namespace cinn {

struct RefCount {
  RefCount() = default;
  RefCount(const RefCount& other) { count.store(other.count); }
  RefCount(RefCount&& other) { count.store(other.count); }
  const RefCount& operator=(const RefCount& other) {
    count.store(other.count);
    return *this;
  }

  void inc() const { ++count; }
  void dec() const { --count; }

  mutable std::atomic<int> count{};
};

class Referenced {
 public:
  mutable RefCount ref_count;

  Referenced() = default;
  Referenced(const Referenced& other) : ref_count(other.ref_count) {}
  Referenced(Referenced&& other) : ref_count(std::move(other.ref_count)) {}
  const Referenced& operator=(const Referenced& other) { ref_count = other.ref_count; }
};

template <typename Referenced>
const RefCount& ref_count(Referenced& x) {
  return x.ref_count;
}

/// T should be a type that inherient from Referenced.
template <typename T>
class RefPointer {
 protected:
  T* ptr_{};

 public:
  RefPointer() = default;
  RefPointer(T* ptr) : ptr_(ptr) {}
  RefPointer(RefPointer&& other) : ptr_(other.ptr_) {}
  RefPointer(const RefPointer& other) : ptr_(other.ptr_) { IncRef(); }
  void operator=(const RefPointer&& other) { ptr_ = other.ptr_; }

  T* get() const { return ptr_; }
  T& operator*() { return *get(); }
  T* operator->() { return ptr_; }
  bool valid() const { return ptr_; }

  ~RefPointer() { DecRef(); }

 private:
  void IncRef() const {
    if (ptr_) {
      ref_count(*ptr_).inc();
    }
  }

  void DecRef() const {
    if (ptr_) {
      ref_count(*ptr_).dec();
    }
  }

  void Destroy() { static_cast<T*>(ptr_)->Destroy(); }
};

}  // namespace cinn
