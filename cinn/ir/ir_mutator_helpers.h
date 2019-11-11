#pragma once

#include "cinn/ir/ir_mutator.h"

namespace cinn {
namespace ir {

class IRVarReplacer : public IRMutator {
  const ir::Var var_;
  const ir::Expr expr_;

 public:
  IRVarReplacer(const ir::Var& var, const ir::Expr& expr) : var_(var), expr_(expr) {}

  void Visit(const ir::Expr* expr, ir::Expr* op) override;

  Expr VisitBasicExpr(Expr* expr) override;
};

/**
 * Replace a expression with another other.
 */
class IRExprReplacer : public IRMutator {
  const ir::Expr& target_;
  std::string source_;

 public:
  IRExprReplacer(const ir::Expr& source, const ir::Expr& target);

  void Visit(const ir::Expr* expr, ir::Expr* op) override { IRMutator::Visit(expr, op); }

  Expr VisitBasicExpr(Expr* expr) override;
};

}  // namespace ir
}  // namespace cinn
