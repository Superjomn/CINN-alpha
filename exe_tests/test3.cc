/**
 * @brief This file includes the test for matrix muplication.
 */
#include <gtest/gtest.h>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

Stage MatMul(Function& fn, Expr& A, Expr& B, Expr& C, Var& m, Var& n, Var& k) {
  Stage s0(C[m][n] += A[m][k] * B[k][n], {m, n, k});
  s0 = fn.AddStage(s0);
  return s0;
}

// A[m,n]
// Bias[n]
Stage AddBias(Function& fn, Expr& A, Expr& Bias, Var& m, Var& n) {
  Stage s0(A[m][n] += Bias[n], {m, n});
  s0 = fn.AddStage(s0);
  return s0;
}

// A[m,n]
// Scale: float32
Stage Scale(Function& fn, Expr& A, Expr& scale, Var& m, Var& n) {
  Stage s0(A[m][n] *= scale, {m, n});
  s0 = fn.AddStage(s0);
  return s0;
}

TEST(cinn, complex_nn) {
  ir::Constant M(512), N(512), K(256);

  Function fn("complex");
  {
    Expr A({M, K}, primitive_t::float32, "A");
    Expr B({K, N}, primitive_t::float32, "B");
    Expr C({M, N}, primitive_t::float32, "C");
    Expr Bias({N}, primitive_t::float32, "Bias");

    Var m("m"), n("n"), k("k");
    Expr scale_ratio(1.3f);

    auto s_init = fn.AddStage(  //
        C[m][n] = Expr(0.f));
    auto s0 = MatMul(fn, A, B, C, m, n, k);
    auto s1 = AddBias(fn, C, Bias, m, n);
    auto s2 = Scale(fn, C, scale_ratio, m, n);

    s2.FuseWith(s1);
    s0.Tile({32, 32});
    s1.TileUnroll({32, 4});
    // s1.Tile({32, 32});
    // s0.Tile(n, 32);
    // s0.Tile(k, 32);
    s0.Interchange(m, k);

    fn.Inputs({A, B, Bias});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  LOG(INFO) << "generated expr:\n" << ir::Dump(Expr(fn));

  backends::CompileAsC(Expr(fn), "exe_test3.h", "exe_test3.cc");
}

}  // namespace cinn
