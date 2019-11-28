#pragma once

#include <ginac/ginac.h>
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
 * Simplify the constant expressions.
 * @param source
 */
void IRSimplify(ir::Expr* source);

/**
 * Remove unnecessary Casts.
 */
void IRCleanRedundantCasts(ir::Expr* expr);

/**
 * Tell if this forloop's init, cond and extent are all constant integers.
 * @param expr
 * @param num_elements
 * @param init_value
 * @return
 */
bool IsConstantFor(const ir::Expr& expr, int* num_elements, int* init_value);

std::ostream& operator<<(std::ostream& os, const ir::Expr& x);

std::ostream& operator<<(std::ostream& os, ir::NodeTy type);

/**
 * Helper to convert cinn::Expr to GiNaC::expr
 */
struct ExprToGinacConveter {
  GiNaC::ex operator()(Expr expr);

  std::string Repr(const Expr& expr);

  GiNaC::symbol CreateGinacSymbol(const std::string& repr);
  GiNaC::symbol CreateGinacSymbol(const ir::Expr& var);

 private:
  GiNaC::ex BuildHelper(ir::Expr expr);

  void RecordExpr(const ir::Expr& expr);

 private:
  std::map<std::string, ir::Expr> repr_to_expr_;
  std::map<std::string, GiNaC::symbol> repr_to_ginac_;
};

/**
 * Tell whether an expression is a basic expression.
 * A basic expression is defined as one without any blocks, and contains only the References, Vars and math operations
 * include + - * / % and so on.
 */
bool IsBasicExpr(ir::Expr expr);

/**
 * Tell whether a expression has the sub-expression and the times is 1.
 * @return
 */
bool BasicExprIdentityVarScale(const Expr& expr, const Expr& var_expr);

bool ReferenceIsAddress(const Expr& expr);

}  // namespace ir
}  // namespace cinn
