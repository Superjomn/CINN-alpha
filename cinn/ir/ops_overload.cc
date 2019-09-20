#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace ir {}  // namespace ir

ir::Expr Tanh(const ir::Expr &e) { return ir::Tanh::make(e); }
ir::Expr Sigmoid(const ir::Expr &e) { return ir::Sigmoid::make(e); }
ir::Expr Max(const ir::Expr &a, const ir::Expr &b) { return ir::Max::make(a, b); }
ir::Expr Min(const ir::Expr &a, const ir::Expr &b) { return ir::Min::make(a, b); }

}  // namespace cinn
