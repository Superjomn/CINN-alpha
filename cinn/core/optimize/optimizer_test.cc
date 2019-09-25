#include "cinn/core/optimize/optimizer.h"
#include <gtest/gtest.h>
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir_printer.h"

namespace cinn {

TEST(Optimizer_pass, indices_to_absolute_offset) {
  ir::Constant N(30), M(40), K(60);
  Expr A({N, M, K}, primitive_t::float32, "A");
  ir::Var i, j, k;

  auto expr = A[i][j][k];
  Optimizer optimizer;
  optimizer(&expr);

  auto log = ir::Dump(expr);
  LOG(INFO) << "ir: " << log;

  ASSERT_EQ(log, "A<>[((((i0 * 30) + i1) * 40) + i2)]");
}

}  // namespace cinn

USE_PASS(indices_to_absolute_offset);
