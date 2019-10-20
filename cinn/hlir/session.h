#pragma once

#include <map>
#include <memory>
#include <string>
#include "cinn/hlir/tensor.h"

namespace cinn {
namespace hlir {

/**
 * Session holds all the Variables and Placeholders.
 */
class Session {
 public:
  /**
   * Get a Tensor.
   * @param name name of the tensor to retrive.
   * @return the address of the corresponding Tensor.
   */
  Tensor* GetTensor(const std::string& name) {
    auto it = tensors_.find(name);
    if (it == std::end(tensors_)) return nullptr;
    return it->second.get();
  }

  /**
   * Create a new Tensor with specific name.
   * @param name name of the Tensor.
   * @return the address of the newly created Tensor.
   */
  Tensor* NewTensor(const std::string& name) {
    CHECK(!tensors_.count(name));
    auto* t = new Tensor;
    tensors_[name].reset(t);
    return t;
  }

  /**
   * Get the count of Tensors in the session.
   * @return the size.
   */
  size_t size() const { return tensors_.size(); }

 private:
  std::map<std::string, std::unique_ptr<Tensor>> tensors_;
};

}  // namespace hlir
}  // namespace cinn
