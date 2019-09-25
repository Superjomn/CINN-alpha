#include "cinn/backends/code_gen_c.h"
#include <gtest/gtest.h>
#include "cinn/core/function.h"
#include "cinn/core/isl_code_gen.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {

using cs = std::vector<ir::Constant>;

TEST(cpp_code_gen, basic) {
  ir::Constant M(100), N(200), K(300);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  ir::Var i("i"), j("j"), k("k");

  Function fn("fn");
  {
    Stage s0 = fn.AddStage(B[i + 1][j].Assign(  //
        (A[i - 1][j] + A[i][j] + A[i + 1][j]) / 3));

    Stage s1 = fn.AddStage(C[i][j].Assign(  //
        A[i][j] * 2 + B[i][j] / 2));

    fn.Inputs({A, B});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  LOG(INFO) << "f0.block:\n" << ir::Dump(fn.GetTransformedExpr());

  std::stringstream os;
  backends::CppCodeGen code_gen(os);
  code_gen.Print(Expr(fn));

  std::string log = os.str();
  LOG(INFO) << "generated code: \n" << log;

  std::string target =
      R"ROC(void fn (const char* A, const char* B)
{

  {
    for(int c0 = 1; (c0 <= 99); c0 += 1)
    {
      for(int c1 = 0; (c1 <= 200); c1 += 1)
      {
                B<>[(c0 + 1),c1] = (((A<>[(c0 - 1),c1] + A<>[c0,c1]) + A<>[(c0 + 1),c1]) / 3);

      }

    }

    for(int c0 = 0; (c0 <= 100); c0 += 1)
    {
      for(int c1 = 0; (c1 <= 200); c1 += 1)
      {
                C<>[c0,c1] = ((A<>[c0,c1] * 2) + (B<>[c0,c1] / 2));

      }

    }

  }
})ROC";

  // ASSERT_EQ(log, target);
}

namespace backends {}  // namespace backends
}  // namespace cinn
