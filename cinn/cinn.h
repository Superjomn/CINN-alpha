/**
 * @brief This file include the necessary header files to make the usage of CINN easier.
 */

#pragma once

#include "cinn/backends/code_gen_c.h"
#include "cinn/core/function.h"
#include "cinn/core/optimize/optimizer.h"
#include "cinn/core/optimize/pass_registry.h"

namespace cinn {

using ir::Constant;
using ir::Expr;
using ir::Var;

}  // namespace cinn
