#pragma once

#include <set>
#include <string>
#include <vector>

#include "cinn/model_loader/tensor.h"
#include "cinn/utils/varient.h"

namespace cinn {
namespace model_loader {

using FeedFetchList = std::vector<Tensor>;

class Variable {
 public:
  template <typename T>
  const T& Get() const {
    return blob_.get<T>();
  }

  template <typename T>
  T* GetMutable() {
    if (!blob_.is<T>()) blob_.set<T>();
    return blob_.get_mutable<T>();
  }

  template <typename T>
  bool IsType() {
    return blob_.type() == typeid(T).hash_code();
  }

 private:
  // variant<int, float, std::string, lite::Tensor> blob_;
  variant<int, float, std::string, Tensor, std::vector<Tensor>> blob_;
};

}  // namespace model_loader
}  // namespace cinn
