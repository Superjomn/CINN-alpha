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

}  // namespace ir
}  // namespace cinn
