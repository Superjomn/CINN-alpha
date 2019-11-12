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

/**
 * Tell whether two expression is the same.
 * @param a the first expression.
 * @param b the second expression.
 * @return boolean that represents whether two expressions equals.
 */
bool IREquals(const Expr& a, const Expr& b);

ir::Expr IRDeepCopy(const Expr& a);

ir::Expr IRReplace(ir::Expr* source, Expr from, ir::Expr to);

}  // namespace ir
}  // namespace cinn
