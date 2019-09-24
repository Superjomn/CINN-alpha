#pragma once

#include "cinn/ir/ir.h"

namespace cinn {
namespace ir {

Expr CopyExpr(const Expr& expr);

std::vector<const Var*> CollectVarsFromExpr(const Expr& expr);

//! Collect the expressions of type `type`.
template <typename T>
std::vector<const T*> CollectExprNode(const Expr& expr);

static Expr tanh(Expr x) { return Max::make(Expr(0.f), x); }
static Expr exp(Expr x) { return Exp::make(x); }

}  // namespace ir
}  // namespace cinn
