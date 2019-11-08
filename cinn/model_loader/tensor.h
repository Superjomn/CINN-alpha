
#pragma once

#include <glog/logging.h>
#include <algorithm>
#include <functional>  // for multiplies
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "cinn/model_loader/host_buffer.h"
#include "cinn/type.h"

namespace cinn {
namespace model_loader {

class DDim {
 public:
  using value_type = int64_t;

  DDim() = default;

  explicit DDim(const std::vector<value_type> &x) { ConstructFrom(x); }
  // DDimLite(std::initializer_list<value_type> init_list) :
  // DDimLite(std::vector<value_type>(init_list)) {}

  void ConstructFrom(const std::vector<value_type> &x) { data_ = x; }

  value_type operator[](int offset) const { return data_[offset]; }
  value_type &operator[](int offset) { return data_[offset]; }
  std::vector<int64_t> Vectorize() const { return data_; }

  size_t size() const { return data_.size(); }
  bool empty() const { return data_.empty(); }

  value_type production() const;

  const std::vector<value_type> &data() const { return data_; }
  value_type count(int start, int end) const;

  DDim Slice(int start, int end) const;

  DDim Flatten2D(int col) const {
    return DDim(std::vector<value_type>({Slice(0, col).production(), Slice(col, size()).production()}));
  }

  std::string repr() const;

  friend std::ostream &operator<<(std::ostream &os, const DDim &dims) {
    os << dims.repr();
    return os;
  }

  friend bool operator==(const DDim &a, const DDim &b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }

  friend bool operator!=(const DDim &a, const DDim &b) { return !(a == b); }

 private:
  std::vector<value_type> data_;
};

using LoD = std::vector<std::vector<uint64_t>>;

// A light-weight tensor implementation.
class Tensor {
 public:
  Tensor() : buffer_(std::make_shared<HostBuffer>()) {}

  void Resize(const DDim &ddim) { dims_ = ddim; }
  void Resize(const std::vector<int64_t> &x) { dims_ = DDim(x); }

  const DDim &dims() const { return dims_; }
  int64_t numel() const { return dims_.production(); }

  const LoD &lod() const { return lod_; }
  LoD *mutable_lod() { return &lod_; }
  void set_lod(const LoD &lod) { lod_ = lod; }

  primitive_t precision() const { return precision_; }
  void set_precision(primitive_t precision) { precision_ = precision; }

  bool persistable() const { return persistable_; }
  void set_persistable(bool persistable) { persistable_ = persistable; }

  void *mutable_data(size_t memory_size);

  template <typename T>
  T *mutable_data() {
    buffer_->Resize(dims().production());
    return static_cast<T *>(buffer_->data());
  }

  template <typename T>
  const T *data() const {
    return static_cast<T *>(buffer_->data());
  }

  const void *raw_data() const { return static_cast<char *>((static_cast<char *>(buffer_->data()) + offset_)); }

  size_t data_size() const { return this->dims().production(); }

  size_t memory_size() const { return memory_size_; }

  size_t offset() const { return offset_; }

  bool is_initialized() const { return buffer_->data(); }

  // Other share data to this.
  void ShareDataWith(const Tensor &other);

  void CopyDataFrom(const Tensor &other);

  template <typename T>
  Tensor Slice(int64_t begin, int64_t end) const;

  friend std::ostream &operator<<(std::ostream &os, const Tensor &tensor) {
    os << "Tensor:" << '\n';
    os << "dim: " << tensor.dims() << '\n';
    for (int i = 0; i < tensor.dims().production(); i++) {
      os << tensor.template data<float>()[i] << " ";
    }
    os << "\n";
    return os;
  }

 private:
  // precision_ and persistable_ are only used for persistable vars.
  // If your tensor wants to be saved and loaded correctly, you must
  // set values of precision_ and persistable_ after updating it.
  // If your tensor is just a temp tensor, such as activations,
  // you can ignore these two attributes.
  primitive_t precision_{primitive_t::unk};
  bool persistable_{false};

  DDim dims_;
  std::shared_ptr<HostBuffer> buffer_;
  LoD lod_;
  size_t memory_size_{};

  /// @brief Buffer may be shared with other tensors
  size_t offset_{0};
};

template <typename T>
Tensor Tensor::Slice(int64_t begin, int64_t end) const {
  CHECK_GE(begin, 0);
  CHECK_LE(end, dims_[0]);
  CHECK_LT(begin, end);
  if (dims_[0] == 1) {
    return *this;
  } else {
    int64_t base = numel() / dims_[0];
    Tensor dst;
    dst.buffer_ = buffer_;
    auto dst_dims = dims_;
    dst_dims[0] = end - begin;
    dst.Resize(dst_dims);
    dst.offset_ = offset_ + static_cast<size_t>(begin * base) * sizeof(T);
    return dst;
  }
}

template <typename TensorT>
bool TensorCompareWith(const TensorT &a, const TensorT &b) {
  if (a.dims() != b.dims()) return false;
  if (memcmp(a.raw_data(), b.raw_data(), a.data_size()) != 0) return false;
  return true;
}

}  // namespace model_loader
}  // namespace cinn
