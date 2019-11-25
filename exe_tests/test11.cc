#include <gtest/gtest.h>
#include <fstream>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

TEST(matmul, basic) {
  SetGlobalContext(new CINNContext);

  ir::Constant M(256), N(256), K(128);

  Function fn("basic");
  {
    Expr A({M, K}, primitive_t::float32, "A");
    Expr B({N, K}, primitive_t::float32, "B");
    Expr C({M, N}, primitive_t::float32, "C");
    Expr C1({M, N}, primitive_t::float32, "C1");
    Expr C2({M, N}, primitive_t::float32, "C2");
    Expr C3({M, N}, primitive_t::float32, "C3");
    Expr bias({N}, primitive_t::float32, "bias");

    Var i("i"), j("j"), k("k");

    auto s0 = fn.AddStage(C[i][j] = Expr(0.f));
    auto s1 = fn.AddStage(C1[i][j] = C[i][j] + A[i][k] * B[j][k]);
    auto s2 = fn.AddStage(C2[i][j] = C1[i][j] + bias[j]);
    auto s3 = fn.AddStage(C3[i][j] = ir::Max_(C2[i][j], Expr(0.f)));

    s2.FuseWith(s3);

    s1.Vectorize({8, 8});

    fn.Inputs({A, B, bias});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  backends::CompileAsC(Expr(fn), "exe_test11.h", "exe_test11.cc");
}

}  // namespace cinn