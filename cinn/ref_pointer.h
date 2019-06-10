#include <atomic>

namespace cinn {

struct RefCount {
  void inc() { ++count; }
  void dec() { --count; }

  std::atomic<int> count;
};

class Referenced {
 public:
  RefCount ref_count;
};

template <typename Referenced>
RefCount& ref_count(Referenced& x) {
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
  T* get() const { return ptr_; }
  T& operator*() { return *get(); }
  T* operator->() { return ptr_; }

  ~RefPointer() { DecRef(); }

  RefPointer(RefPointer&& other) : ptr_(other.ptr_) {}

  RefPointer(const RefPointer& other) : ptr_(other.ptr_) { IncRef(); }

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
