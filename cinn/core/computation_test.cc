#include "cinn/core/computation.h"
#include <gtest/gtest.h>
#include "cinn/ir/ops_overload.h"

namespace cinn {

TEST(computation, Construct) {
  Computation comp("comp1", "[n] -> { A[i,j] : 0 < i < n and 0 < j < n}");
  comp.ApplyTransformationOnScheduleDomain("[n] -> {A[i,j] -> [j, i+1]}");
  ir::Parameter N("N", 100);

  ir::Var i("i"), j("j"), i0("i0"), j0("j0"), i1("i1"), j1("j1");
  ir::Expr e1 = ir::Expr((int32_t)1) + ir::Expr((int32_t)2);
}

}  // namespace cinn