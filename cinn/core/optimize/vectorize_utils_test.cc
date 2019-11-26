#include "cinn/core/optimize/vectorize_utils.h"
#include <gtest/gtest.h>

namespace cinn {
namespace optimize {

TEST(BasicExprContainsOnlySIMDReleatedOpr, true_case) {
  SetGlobalContext(new CINNContext);

  ir::Constant M("M", 100);
  ir::Constant N("M", 100);
  ir::Var i, j;
  Expr A({M, N}, primitive_t::float32);
  Expr B({M, N}, primitive_t::float32);
  Expr C({M, N}, primitive_t::float32);

  ASSERT_TRUE(BasicExprContainsOnlySIMDReleatedOpr(A[i][j] + B[i][j]));
  ASSERT_TRUE(BasicExprContainsOnlySIMDReleatedOpr(A[i][j] - B[i][j]));
  ASSERT_TRUE(BasicExprContainsOnlySIMDReleatedOpr(A[i][j] * B[i][j]));
  ASSERT_TRUE(BasicExprContainsOnlySIMDReleatedOpr(A[i][j] / B[i][j]));
  ASSERT_TRUE(BasicExprContainsOnlySIMDReleatedOpr(C[i][j] = A[i][j] / B[i][j]));
}

TEST(BasicExprContainsOnlySIMDReleatedOpr, false_case) {
  SetGlobalContext(new CINNContext);

  ir::Constant M("M", 100);
  ir::Constant N("M", 100);
  ir::Var i, j;
  Expr A({M, N}, primitive_t::float32);
  Expr B({M, N}, primitive_t::float32);
  Expr C({M, N}, primitive_t::float32);

  std::vector<ir::Expr> exprs({
      A[i][j] % B[i][j],  //
      A[i][j] += B[i][j]  //
  });

  for (auto& expr : exprs) {
    ASSERT_FALSE(BasicExprContainsOnlySIMDReleatedOpr(expr));
  }
}

TEST(BasicExprVarsCanPassToSIMD, test) {
  SetGlobalContext(new CINNContext);

  ir::Constant M("M", 100);
  ir::Constant N("M", 100);
  ir::Var i, j;
  Expr A({M, N}, primitive_t::float32);
  Expr B({M, N}, primitive_t::float32);
  Expr C({M, N}, primitive_t::float32);

  std::vector<std::tuple<ir::Expr, bool>> exprs({
      std::make_tuple(A[i][j] + B[i][j], true),   //
      std::make_tuple(A[i][j] + B[i][j], true),   //
      std::make_tuple(A[j][i] + B[i][j], false),  //
      std::make_tuple(A[j][j] + B[i][j], false),  //
  });

  for (auto& expr : exprs) {
    LOG(INFO) << "testing " << std::get<0>(expr);
    if (std::get<1>(expr))
      ASSERT_TRUE(BasicExprVarsCanPassToSIMD(std::get<0>(expr), j));
    else
      ASSERT_FALSE(BasicExprVarsCanPassToSIMD(std::get<0>(expr), j));
  }
}

}  // namespace optimize
}  // namespace cinn
