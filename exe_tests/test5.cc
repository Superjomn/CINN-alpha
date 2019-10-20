/**
 * @brief This file includes the test for conv and pooling.
 */
#include <gtest/gtest.h>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

TEST(test5, basic) {
  Function fn("conv");
  {
    Constant N(1);
    Constant C(20);
    Constant H(100);
    Constant W(200);
    Constant M(10);

    Constant KH(4);
    Constant KW(4);

    Expr I({N, C, H, W}, primitive_t::float32, "I");
    Expr W1({M, C, KH, KW}, primitive_t::float32, "W1");
    Expr O({N, C, H, W}, primitive_t::float32, "O");

    Var n("n"), m("m"), h("h"), w("w");
    Var r_c("rc"), r_kh("rkh"), r_kw("rkw");

    fn.AddStage(  //
        O[n][m][h][w] += I[n][r_c][h + r_kh][w + r_kw] * W1[m][r_c][r_kh][r_kw]);

    // O(n, m, h, w) +=! I(n, r_c, h + r_kh, w + r_kw) * W1(m, r_c, r_kh, r_kw)
    // O(n, m, h, w)  = O(n, m, h, w) + Bias(m)

    fn.Inputs({I, W1});
    fn.Outputs({O});

    fn.EndDefinition();
  }

  backends::CompileAsC(Expr(fn), "exe_test5.h", "exe_test5.cc");
}

}  // namespace cinn
