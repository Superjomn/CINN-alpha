#include "cinn/ir/ir_helper.h"
#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>
#include "cinn/core/stage.h"

namespace cinn {
namespace ir {

TEST(ir, equal) {
  SetGlobalContext(new CINNContext);

  Expr a(1.f);
  Expr b(2.f);

  Expr a1 = a;
  Expr b1(2.f);

  ASSERT_FALSE(IREquals(a, b));
  ASSERT_TRUE(IREquals(a + b, a1 + b1));
}

TEST(ir, basic_simplify) {
  SetGlobalContext(new CINNContext);

  using tuple_t = std::tuple<ir::Expr, std::string>;

  Expr a(1.f);
  Expr b(0.f);
  Expr c(2.2f);
  Expr c2(2.f);
  Expr x("x", primitive_t::float32);

  std::vector<tuple_t> tests;
  tests.emplace_back(std::make_tuple(a - b, "1"));
  tests.emplace_back(std::make_tuple(a - b + c, "3.2"));
  tests.emplace_back(std::make_tuple(a - b - c, "-1.2"));
  tests.emplace_back(std::make_tuple(a * b, "0"));
  tests.emplace_back(std::make_tuple(a * b - c, "-2.2"));
  tests.emplace_back(std::make_tuple(b / a, "0"));
  tests.emplace_back(std::make_tuple(x / a, "x"));
  tests.emplace_back(std::make_tuple(x * a, "x"));
  tests.emplace_back(std::make_tuple(x * (c2 - a), "x"));
  tests.emplace_back(std::make_tuple(a * x, "x"));
  tests.emplace_back(std::make_tuple(x / (c2 - a), "x"));
  tests.emplace_back(std::make_tuple(x / (c2 - c), "(x / -0.2)"));
  tests.emplace_back(std::make_tuple(x / (c2 - c + c), "(x / 2)"));
  // tests.emplace_back(std::make_tuple(x - c + c, "x"));

  for (auto& test : tests) {
    auto& expr = std::get<0>(test);
    std::string repr = GetStreamStr(expr);
    IRSimplify(&expr);
    LOG(INFO) << "simplify " << repr << " -> " << expr;
    EXPECT_EQ(GetStreamStr(expr), std::get<1>(test));
  }
}

TEST(ir, reference_simplify) {
  SetGlobalContext(new CINNContext);

  Constant M(20);
  Constant K(10);
  Constant N(30);
  Expr A({M, K}, primitive_t::float32, "A");
  Expr B({K, N}, primitive_t::float32, "B");
  Expr C({M, N}, primitive_t::float32, "C");

  Var i("i"), j("j");

  Stage s0 = A[i][j * Expr(0)].Assign(B[i + Expr(0)][j] + C[i][j + 0]);

  Expr e = s0.expr();
  IRSimplify(&e);

  LOG(INFO) << "expr: " << e;
  EXPECT_EQ(GetStreamStr(e), "A<20,10>[i,0] = (B<10,30>[i,j] + C<20,30>[i,j]);");
}

TEST(ir, replace_basic_expr) {
  SetGlobalContext(new CINNContext);

  Constant M(10), N(20);
  Expr C({M, N}, primitive_t::float32, "C");
  Expr A({M, N}, primitive_t::float32, "A");
  Var i("i", primitive_t::int32), j("j", primitive_t::int32);
  Expr expr = ir::Assign::make(C[i][j], A[i][j] * Expr(2.f) + Expr(1.f));
  LOG(INFO) << "expr: " << expr;

  // replace A[i][j]*2 -> (A[i][j]+1)
  Expr from = A[i][j] * 2.f;
  Expr to = A[i][j] + 1.f;

  IRReplace(&expr, from, to);

  LOG(INFO) << "after replace: " << expr;

  EXPECT_EQ(GetStreamStr(expr), "C<10,20>[i,j] = ((A<10,20>[i,j] + 1) + 1);");
}

}  // namespace ir
}  // namespace cinn
