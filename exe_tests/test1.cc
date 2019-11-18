/**
 * @brief This file includes the test for matrix muplication.
 */
#include <gtest/gtest.h>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

TEST(cinn, mat_mul) {
  SetGlobalContext(new CINNContext);

  ir::Constant M(100), N(200), K(150);

  Function fn("mat_mul");
  {
    Expr A({M, K}, primitive_t::float32, "A");
    Expr B({K, N}, primitive_t::float32, "B");
    Expr C({M, N}, primitive_t::float32, "C");

    Var m, n, k;

    auto s0 = fn.AddStage(  //
        C[m][n].Assign(C[m][n] + A[m][k] * B[k][n]));

    fn.Inputs({A, B});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  LOG(INFO) << "generated expr:\n" << ir::Dump(Expr(fn));

  backends::CompileAsC(Expr(fn), "exe_test1.h", "exe_test1.cc");
}

}  // namespace cinn
