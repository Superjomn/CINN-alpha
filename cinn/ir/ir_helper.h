#pragma once

#include "cinn/ir/ir.h"

namespace cinn {
namespace ir {

Expr CopyExpr(const Expr& expr);

std::vector<const Var*> CollectVarsFromExpr(const Expr& expr);

//! Collect the expressions of type `type`.
template <typename T>
std::vector<const T*> CollectExprNode(const Expr& expr);

}  // namespace ir
}  // namespace cinn
