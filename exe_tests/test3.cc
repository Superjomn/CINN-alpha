/**
 * @brief This file includes the test for matrix muplication.
 */
#include <gtest/gtest.h>
#include "cinn/cinn.h"

namespace cinn {

Stage MatMul(Function& fn, Expr& A, Expr& B, Expr& C, Var& m, Var& n, Var& k) {
  auto s0 = fn.AddStage(  //
      C[m][n].Assign(C[m][n] + A[m][k] * B[k][n]));
  return s0;
}

// A[m,n]
// Bias[n]
Stage AddBias(Function& fn, Expr& A, Expr& Bias, Var& m, Var& n) {
  auto s0 = fn.AddStage(  //
      A[m][n] = A[m][n] + Bias[n]);
  return s0;
}

// A[m,n]
// Scale: float32
Stage Scale(Function& fn, Expr& A, Expr& scale, Var& m, Var& n) {
  auto s0 = fn.AddStage(A[m][n] = A[m][n] * scale);
  return s0;
}

TEST(cinn, complex_nn) {
  ir::Constant M(100), N(200), K(150);

  Function fn("complex");
  {
    Expr A({M, K}, primitive_t::float32, "A");
    Expr B({K, N}, primitive_t::float32, "B");
    Expr C({M, N}, primitive_t::float32, "C");
    Expr Bias({N}, primitive_t::float32, "Bias");

    Var m("m"), n("n"), k("k");
    Expr scale_ratio(1.3f);

    auto s0 = MatMul(fn, A, B, C, m, n, k);
    auto s1 = AddBias(fn, C, Bias, m, n);
    auto s2 = Scale(fn, C, scale_ratio, m, n);

    s2.FuseWith(s1);
    // s1.FuseWith(s0);
    s0.Tile(m, 32);
    s0.Tile(n, 32);

    s1.Tile(m, 4);
    s2.Tile(m, 4);

    fn.Inputs({A, B, Bias});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  LOG(INFO) << "generated expr:\n" << ir::Dump(Expr(fn));

  backends::CompileAsC(Expr(fn), "exe_test3.h", "exe_test3.cc");
}

}  // namespace cinn

USE_PASS(indices_to_absolute_offset);
