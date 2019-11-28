#include "cinn/ir/expr.h"
#include "cinn/core/function.h"

namespace cinn {
namespace ir {

#define __(type__)                                                                               \
  template <>                                                                                    \
  ir::type__* IRHandle::As() {                                                                   \
    if (type() == ir::NodeTy::type__ && ptr_) return static_cast<ir::type__*>(ptr_.get());       \
    return nullptr;                                                                              \
  }                                                                                              \
  template <>                                                                                    \
  const ir::type__* IRHandle::As() const {                                                       \
    if (type() == ir::NodeTy::type__ && ptr_) return static_cast<const ir::type__*>(ptr_.get()); \
    return nullptr;                                                                              \
  }
NODETY_FOR_EACH(__)
#undef __

NodeTy IRHandle::type() const {
  CHECK(ptr_);
  return ptr_->type();
}

}  // namespace ir
}  // namespace cinn
