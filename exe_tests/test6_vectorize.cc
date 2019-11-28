#include <gtest/gtest.h>
#include <fstream>
#include "cinn/cinn.h"
#include "cinn/core/optimize/use_passes.h"

namespace cinn {

TEST(test, basic) {
  SetGlobalContext(new CINNContext);

  ir::Constant M(100), N(200), O(150);

  Function fn("basic");
  {
    Expr A({M, N}, primitive_t::float32, "A");
    Expr B({M, N}, primitive_t::float32, "B");
    Expr C({M, N}, primitive_t::float32, "C");

    Var i("i"), j("j");

    auto s0 = fn.AddStage(C[i][j] = (A[i][j] + B[i][j]) * B[i][j]);
    auto s1 = fn.AddStage(C[i][j] = (A[i][j] + B[i][j]) * B[i][j]);
    // auto s1 = fn.AddStage(C[i][j] += Expr(1.f));
    s0.Vectorize({8, 8});
    s1.Vectorize({8, 8});

    fn.Inputs({A, B});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  backends::CompileAsC(Expr(fn), "exe_test6.h", "exe_test6.cc");

  {
    std::fstream file("exe_test6.cc");
    std::string line;
    std::stringstream ss;
    while (std::getline(file, line)) {
      ss << line << '\n';
    }

    std::string target = R"ROC(
void basic (cinn_float32_t* A, cinn_float32_t* B, cinn_float32_t* C) {
  // vectorize - tiles
  for (int c0 = 0; (c0 <= 11); c0 += 8) {
    for (int c1 = 0; (c1 <= 24); c1 += 8) {
      // vectorize - points
      _mm256_store_ps(&C[((c0 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((c0 * 200) + c1)]), _mm256_load_ps(&B[((c0 * 200) + c1)])), _mm256_load_ps(&B[((c0 * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 1) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 1) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 1) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 1) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 2) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 2) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 2) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 2) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 3) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 3) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 3) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 3) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 4) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 4) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 4) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 4) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 5) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 5) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 5) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 5) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 6) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 6) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 6) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 6) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 7) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 7) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 7) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 7) * 200) + c1)])));
    }
    for (int c1 = 32; (c1 <= 199); c1 += 8) {
      // vectorize - points
      _mm256_store_ps(&C[((c0 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((c0 * 200) + c1)]), _mm256_load_ps(&B[((c0 * 200) + c1)])), _mm256_load_ps(&B[((c0 * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 1) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 1) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 1) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 1) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 2) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 2) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 2) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 2) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 3) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 3) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 3) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 3) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 4) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 4) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 4) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 4) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 5) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 5) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 5) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 5) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 6) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 6) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 6) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 6) * 200) + c1)])));
      _mm256_store_ps(&C[(((c0 + 7) * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(((c0 + 7) * 200) + c1)]), _mm256_load_ps(&B[(((c0 + 7) * 200) + c1)])), _mm256_load_ps(&B[(((c0 + 7) * 200) + c1)])));
    }
  }
)ROC";

    EXPECT_TRUE(Contains(ss.str(), Trim(target)));
  }
}

}  // namespace cinn
