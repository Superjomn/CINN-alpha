#include "cinn/core/computation.h"
#include <gtest/gtest.h>
#include "cinn/ir/ops_overload.h"

namespace cinn {

TEST(computation, Construct) {
  Computation comp("comp1", "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  comp.ApplyTransformationOnScheduleRange("[T,N] -> {S[o0,t', o2,i'] -> S[o0,t'+10,o2,i'] }");
  LOG(INFO) << comp.Dump();

  /*
  ir::Parameter N("N", 100);

  ir::Var i("i"), j("j"), i0("i0"), j0("j0"), i1("i1"), j1("j1");
  ir::Expr e1 = ir::Expr((int32_t)1) + ir::Expr((int32_t)2);
   */
}

}  // namespace cinn