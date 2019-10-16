#include "cinn/ir/expr.h"
#include "cinn/core/function.h"

namespace cinn {
namespace ir {

template <>
const cinn::Function* IRHandle::As<cinn::Function>() const {
  // TODO(Superjomn) check the type
  if (ptr_) return static_cast<const cinn::Function*>(ptr_.get());
  return nullptr;
}

}  // namespace ir
}  // namespace cinn
