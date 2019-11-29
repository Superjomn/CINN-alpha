#include <gtest/gtest.h>
#include <fstream>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

TEST(test, basic) {
  SetGlobalContext(new CINNContext);

  ir::Constant M(100), N(200), K(160);

  Function fn("basic");
  {
    Expr A({M, K}, primitive_t::float32, "A");
    Expr B({N, K}, primitive_t::float32, "B");
    Expr bias({N}, primitive_t::float32, "bias");
    Expr C({M, N}, primitive_t::float32, "C");

    Var i("i"), j("j"), k("k");

    auto s0 = fn.AddStage(C[i][j] += A[i][k] * B[j][k]);
    auto s1 = fn.AddStage(C[i][j] = C[i][j] + bias[j]);
    auto s2 = fn.AddStage(C[i][j] = ir::Tanh_(C[i][j]));

    // s1.FuseWith(s2);
    // s0.Vectorize({16, 8});
    s0.Vectorize({8});
    s1.Vectorize({16, 8});

    fn.Inputs({A, B, bias});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  backends::CompileAsC(Expr(fn), "exe_test12.h", "exe_test12.cc");
}
}
