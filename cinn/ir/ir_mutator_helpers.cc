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

IRExprReplacer::IRExprReplacer(const ir::Expr &source, const ir::Expr &target) : target_(target) {
  source_ = ir::Dump(source);
}

Expr IRExprReplacer::VisitBasicExpr(Expr *expr) {
  if (ir::Dump(*expr) == source_) {
    *expr = target_;
  }
}

}  // namespace ir
}  // namespace cinn
