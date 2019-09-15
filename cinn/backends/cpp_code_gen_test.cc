#include "cinn/backends/cpp_code_gen.h"
#include <gtest/gtest.h>
#include "cinn/core/function.h"
#include "cinn/core/isl_code_gen.h"
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

  auto f0 = Function::make("f0", {A, B}, {C}, {s0, s1});
  f0->GenerateIslAst();

  LOG(INFO) << "f0.block:\n" << ir::Dump(f0->GetTransformedExpr());

  std::stringstream os;
  backends::CppCodeGen code_gen(os);
  code_gen.Print(Expr(f0));

  std::string log = os.str();
  LOG(INFO) << "generated code: \n" << log;

  std::string target =
      "void f0(char* A, char* B, char * C)\n{\n    for(int c0 = 0; (c0 <= 99); c0 += 1)\n    {\n        for(int c1 = "
      "0; (c1 <= 199); c1 += 1)\n        {\n\n            {\n                C(c0,c1) = ((A(c0,c1) * 2) + (B(c0,c1) / "
      "2));\n                B((c0 + 1),c1) = (((A((c0 - 1),c1) + A(c0,c1)) + A((c0 + 1),c1)) / 3);\n            }\n   "
      "     }\n    }\n}";

  ASSERT_EQ(log, target);
}

namespace backends {}  // namespace backends
}  // namespace cinn
