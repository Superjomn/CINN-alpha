#pragma once

#include "cinn/ir/ir.h"

namespace cinn {
namespace ir {

Expr CopyExpr(const Expr& expr);

std::vector<const Var*> CollectVarsFromExpr(const Expr& expr);

}  // namespace ir
}  // namespace cinn
