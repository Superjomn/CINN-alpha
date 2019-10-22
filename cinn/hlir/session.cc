#include "cinn/hlir/session.h"

namespace cinn {
namespace hlir {

Tensor *Session::GetTensor(const std::string &name) const {
  auto it = tensors_.find(name);
  if (it == std::end(tensors_)) return nullptr;
  CHECK_EQ(it->second->name(), name);
  return it->second.get();
}

Tensor *Session::NewTensor(const std::string &name) {
  CHECK(!tensors_.count(name));
  auto *t = new Tensor;
  t->set_name(name);
  tensors_[name].reset(t);
  return t;
}

}  // namespace hlir
}  // namespace cinn
