/**
 * @brief This file includes the test for conv and pooling.
 */
#include <gtest/gtest.h>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

std::tuple<Stage, Stage> FC(Function& fn, Expr& x, Expr& w, Expr& bias, Expr& out, Var& i, Var& j, Var& k) {
  Stage s0 = fn.AddStage(  //
      out[i][j] = x[i][k] * w[k][j]);
  Stage s1 = fn.AddStage(  //
      out[i][j] = out[i][j] + bias[j]);
  return std::make_tuple(s0, s1);
}

std::tuple<Stage, Stage> SoftMax(Function& fn, Expr& x, Expr& sum, Expr& out, Var& m, Var& n) {
  Stage s0 = fn.AddStage(  //
      sum[Expr(0)] = sum[Expr(0)] + Exp_(x[m][n]));
  Stage s1 = fn.AddStage(  //
      out[m][n].Assign(Exp_(x[m][n]) / sum[Expr(0)]));
  return std::make_tuple(s0, s1);
}

TEST(cinn, complex_nn) {
  ir::Constant M(100), N(200), O(150);

  Function fn("complex");
  {
    Expr sum({1}, primitive_t::float32, "sum");
    Expr x({M, N}, primitive_t::float32, "x");
    Expr w({N, O}, primitive_t::float32, "w");
    Expr w1({O, O}, primitive_t::float32, "w1");
    Expr bias({O}, primitive_t::float32, "bias");
    Expr bias1({O}, primitive_t::float32, "bias1");
    Expr softmax({M, O}, primitive_t::float32, "out");
    Expr output({N, O}, primitive_t::float32, "out1");

    Var m, n, o, o1, o2;

    Stage s0, s1, s2, s3, s4, s5;

    // x: [M,N], m,n
    // w: [N, O], n, o
    // bias: [O], o
    // softmax: [M, O], m, o
    std::tie(s0, s1) = FC(fn, x, w, bias, softmax, m, n, o);
    // softmax: [M, O], m, o
    // w1: [O, O], o, o1
    // bias1, [O], o1
    std::tie(s2, s3) = FC(fn, softmax, w1, bias1, output, m, o, o1);
    // output: [O, O], o1, o2
    std::tie(s4, s5) = SoftMax(fn, output, sum, output, o1, o2);

    // s0.FuseWith(s1);
    s1.FuseWith(s2);
    s2.FuseWith(s3);
    s3.FuseWith(s4);
    s4.FuseWith(s5);

    fn.Inputs({x, w, w1, bias, bias1});
    fn.Outputs({sum, softmax, output});

    fn.EndDefinition();
  }

  LOG(INFO) << "generated expr:\n" << ir::Dump(Expr(fn));

  backends::CompileAsC(Expr(fn), "exe_test4.h", "exe_test4.cc");
}

}  // namespace cinn
