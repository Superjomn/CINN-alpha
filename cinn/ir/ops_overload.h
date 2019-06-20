#pragma once
#include "cinn/ir/ir.h"

/*
 * This file defines various operator overload the utility operators such as '+' '-' '*' '/' to make it easier to use
 * IR.
 */

namespace cinn {
namespace ir {

inline Expr operator+(Expr a, Expr b) { return Add::make(a, b); }

}  // namespace ir
}  // namespace cinn
