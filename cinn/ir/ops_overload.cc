#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace ir {

ir::Expr Tanh_(const ir::Expr &e) { return ir::Tanh::make(e); }
ir::Expr Sigmoid_(const ir::Expr &e) { return ir::Sigmoid::make(e); }
ir::Expr Max_(const ir::Expr &a, const ir::Expr &b) { return ir::Max::make(a, b); }
ir::Expr Min_(const ir::Expr &a, const ir::Expr &b) { return ir::Min::make(a, b); }
ir::Expr Exp_(const ir::Expr &a) { return ir::Exp::make(a); }

}  // namespace ir
}  // namespace cinn
