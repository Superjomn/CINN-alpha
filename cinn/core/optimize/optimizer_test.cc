#include "cinn/core/optimize/optimizer.h"
#include <gtest/gtest.h>
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir_printer.h"

namespace cinn {

TEST(Optimizer_pass, indices_to_absolute_offset) {
  SetGlobalContext(new CINNContext);

  ir::Constant N(30), M(40), K(60);
  Expr A({N, M, K}, primitive_t::float32, "A");
  ir::Var i, j, k;

  auto expr = A[i][j][k];
  IrOptimizer optimizer;
  optimizer(&expr);

  auto log = ir::Dump(expr);
  LOG(INFO) << "ir: " << log;

  auto target = "A<30,40,60>[((((((i0 * 40) * 60) + i1) * 40) * 60) + i2)]";
  ASSERT_EQ(log, target);
}

}  // namespace cinn
