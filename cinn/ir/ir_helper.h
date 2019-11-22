#pragma once

#include <vector>
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace ir {

Expr CopyExpr(const Expr& expr);

std::vector<const Var*> CollectVarsFromExpr(const Expr& expr);

//! Collect the expressions of type `type`.
template <typename T>
std::vector<Expr> CollectExprNode(const Expr& expr);

static Expr tanh(Expr x) { return Max::make(Expr(0.f), x); }
static Expr exp(Expr x) { return Exp::make(x); }

/**
 * Tell whether two expression is the same.
 * @param a the first expression.
 * @param b the second expression.
 * @return boolean that represents whether two expressions equals.
 */
bool IREquals(const Expr& a, const Expr& b);

/**
 * Deep copy a IR expression.
 * @param a the expression to copy.
 * @return A new expression.
 */
ir::Expr IRDeepCopy(const Expr& a);

/**
 * NOTE: some bug here!
 * Replace a specific expression to the other.
 * @param source
 * @param from
 * @param to
 * @return
 */
ir::Expr IRReplace(ir::Expr* source, Expr from, ir::Expr to);

/**
 * Count the occurence of target expression in a expression.
 * @param context the expression to count.
 * @param target the target to search.
 * @return the count.
 */
int IRCount(const Expr& context, const Expr& target);

/**
 * Simplify a IR expression.
 * @param source
 */
void IRSimplify(ir::Expr* source);

std::ostream& operator<<(std::ostream& os, const ir::Expr& x);

std::ostream& operator<<(std::ostream& os, ir::NodeTy type);

}  // namespace ir
}  // namespace cinn
