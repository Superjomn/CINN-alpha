#include "cinn/backends/cpp_code_gen.h"
#include <gtest/gtest.h>
#include "cinn/core/code_gen.h"
#include "cinn/core/function.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {

TEST(cpp_code_gen, basic) {
  Expr A("A"), B("B"), C("C");
  ir::Var i("i", 0, 100), j("j", 0, 200), k("k", 0, 300);
  Stage s0 = B[i + 1][j].Assign(  //
      (A[i - 1][j] + A[i][j] + A[i + 1][j]) / 3);

  Stage s1 = C[i][j].Assign(  //
      A[i][j] * 2 + B[i][j] / 2);

  auto f0 = Function::make("f0", {A, B}, {C}, {&s0, &s1});
  LOG(INFO) << "f0.block:\n" << ir::Dump(f0->GetTransformedExpr());

  std::stringstream os;
  backends::CppCodeGen code_gen(os);
  code_gen.Print(Expr(f0));
  LOG(INFO) << "generated code: \n" << os.str();
}

namespace backends {}  // namespace backends
}  // namespace cinn
