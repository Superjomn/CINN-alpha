#include "cinn/ir/ir_mutator_helpers.h"
#include "cinn/ir/ir_printer.h"

namespace cinn {
namespace ir {

void IRVarReplacer::Visit(const ir::Expr *expr, ir::Expr *op) { IRMutator::Visit(expr, op); }

Expr IRVarReplacer::VisitBasicExpr(Expr *expr) {
  if (expr->is_var() && *expr->As<Var>() == var_) {
    LOG(INFO) << "Replacing " << ir::Dump(*expr) << " to " << ir::Dump(expr_);
    *expr = expr_;
  } else {
    return *expr;
  }
}

}  // namespace ir
}  // namespace cinn
