#include "cinn/model_loader/tensor.h"

namespace cinn {
namespace model_loader {

using value_type = int64_t;

value_type DDim::production() const {
  value_type res = 1;
  for (size_t i = 0; i < this->size(); i++) {
    res *= (*this)[i];
  }
  return res;
}

value_type DDim::count(int start, int end) const {
  if (start < 0) {
    start = 0;
  }
  if (end > size()) {
    end = size();
  }
  if (end < start) {
    end = start;
  }
  value_type sum = 1;
  for (auto i = start; i < end; ++i) {
    sum *= data_[i];
  }
  return sum;
}

DDim DDim::Slice(int start, int end) const {
  std::vector<value_type> vec;
  for (int i = start; i < end; i++) {
    vec.push_back((*this)[i]);
  }
  return DDim(vec);
}

std::string DDim::repr() const {
  std::stringstream ss;
  if (empty()) {
    ss << "{}";
    return ss.str();
  }
  ss << "{";
  for (size_t i = 0; i < this->size() - 1; i++) {
    ss << (*this)[i] << ",";
  }
  if (!this->empty()) ss << (*this)[size() - 1];
  ss << "}";
  return ss.str();
}

void Tensor::ShareDataWith(const Tensor &other) {
  buffer_ = other.buffer_;
  dims_ = other.dims_;
  lod_ = other.lod_;
  memory_size_ = other.memory_size_;
}

void Tensor::CopyDataFrom(const Tensor &other) {
  dims_ = other.dims_;
  lod_ = other.lod_;
  memory_size_ = other.memory_size_;
  memcpy(buffer_->data(), other.buffer_->data(), memory_size_);
}

void *Tensor::mutable_data(size_t memory_size) {
  memory_size_ = memory_size;
  buffer_->Resize(memory_size);
  return buffer_->data();
}

}  // namespace model_loader
}  // namespace cinn
