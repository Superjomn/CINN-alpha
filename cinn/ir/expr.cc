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

NODETY_OP_FOR_EACH(__)
NODETY_CONTROL_OP_FOR_EACH(__)
NODETY_MATH_FUNCTION_FOR_EACH(__)

__(Module)
__(BufferOpr)
__(Reference)
__(SIMDOpr)
__(Cast)
__(Array)
__(Tensor)
__(Var)
__(Mark)
__(IntImm)
__(FloatImm)
__(Allocate)

#undef __

}  // namespace ir
}  // namespace cinn
