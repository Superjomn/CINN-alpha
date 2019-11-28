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
    auto s1 = fn.AddStage(C[i][j] += Expr(1.f));
    s0.Vectorize({16, 8});

    fn.Inputs({A, B});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  backends::CompileAsC(Expr(fn), "exe_test9.h", "exe_test9.cc");

  {
    std::fstream file("exe_test9.cc");
    CHECK(file.is_open());
    std::string line;
    std::stringstream ss;
    while (std::getline(file, line)) {
      ss << line << '\n';
    }
    file.close();

    std::string target = R"ROC(
void basic (cinn_float32_t* A, cinn_float32_t* B, cinn_float32_t* C) {
  // vectorize - tiles
  for (int c1 = 0; (c1 <= 24); c1 += 8) {
    // vectorize - points
    _mm256_store_ps(&C[c1], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[c1]), _mm256_load_ps(&B[c1])), _mm256_load_ps(&B[c1])));
    _mm256_store_ps(&C[(200 + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(200 + c1)]), _mm256_load_ps(&B[(200 + c1)])), _mm256_load_ps(&B[(200 + c1)])));
    _mm256_store_ps(&C[((2 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((2 * 200) + c1)]), _mm256_load_ps(&B[((2 * 200) + c1)])), _mm256_load_ps(&B[((2 * 200) + c1)])));
    _mm256_store_ps(&C[((3 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((3 * 200) + c1)]), _mm256_load_ps(&B[((3 * 200) + c1)])), _mm256_load_ps(&B[((3 * 200) + c1)])));
    _mm256_store_ps(&C[((4 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((4 * 200) + c1)]), _mm256_load_ps(&B[((4 * 200) + c1)])), _mm256_load_ps(&B[((4 * 200) + c1)])));
    _mm256_store_ps(&C[((5 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((5 * 200) + c1)]), _mm256_load_ps(&B[((5 * 200) + c1)])), _mm256_load_ps(&B[((5 * 200) + c1)])));
    _mm256_store_ps(&C[((6 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((6 * 200) + c1)]), _mm256_load_ps(&B[((6 * 200) + c1)])), _mm256_load_ps(&B[((6 * 200) + c1)])));
    _mm256_store_ps(&C[((7 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((7 * 200) + c1)]), _mm256_load_ps(&B[((7 * 200) + c1)])), _mm256_load_ps(&B[((7 * 200) + c1)])));
    _mm256_store_ps(&C[((8 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((8 * 200) + c1)]), _mm256_load_ps(&B[((8 * 200) + c1)])), _mm256_load_ps(&B[((8 * 200) + c1)])));
    _mm256_store_ps(&C[((9 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((9 * 200) + c1)]), _mm256_load_ps(&B[((9 * 200) + c1)])), _mm256_load_ps(&B[((9 * 200) + c1)])));
    _mm256_store_ps(&C[((10 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((10 * 200) + c1)]), _mm256_load_ps(&B[((10 * 200) + c1)])), _mm256_load_ps(&B[((10 * 200) + c1)])));
    _mm256_store_ps(&C[((11 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((11 * 200) + c1)]), _mm256_load_ps(&B[((11 * 200) + c1)])), _mm256_load_ps(&B[((11 * 200) + c1)])));
    _mm256_store_ps(&C[((12 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((12 * 200) + c1)]), _mm256_load_ps(&B[((12 * 200) + c1)])), _mm256_load_ps(&B[((12 * 200) + c1)])));
    _mm256_store_ps(&C[((13 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((13 * 200) + c1)]), _mm256_load_ps(&B[((13 * 200) + c1)])), _mm256_load_ps(&B[((13 * 200) + c1)])));
    _mm256_store_ps(&C[((14 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((14 * 200) + c1)]), _mm256_load_ps(&B[((14 * 200) + c1)])), _mm256_load_ps(&B[((14 * 200) + c1)])));
    _mm256_store_ps(&C[((15 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((15 * 200) + c1)]), _mm256_load_ps(&B[((15 * 200) + c1)])), _mm256_load_ps(&B[((15 * 200) + c1)])));
  }
  for (int c1 = 32; (c1 <= 199); c1 += 8) {
    // vectorize - points
    _mm256_store_ps(&C[c1], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[c1]), _mm256_load_ps(&B[c1])), _mm256_load_ps(&B[c1])));
    _mm256_store_ps(&C[(200 + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[(200 + c1)]), _mm256_load_ps(&B[(200 + c1)])), _mm256_load_ps(&B[(200 + c1)])));
    _mm256_store_ps(&C[((2 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((2 * 200) + c1)]), _mm256_load_ps(&B[((2 * 200) + c1)])), _mm256_load_ps(&B[((2 * 200) + c1)])));
    _mm256_store_ps(&C[((3 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((3 * 200) + c1)]), _mm256_load_ps(&B[((3 * 200) + c1)])), _mm256_load_ps(&B[((3 * 200) + c1)])));
    _mm256_store_ps(&C[((4 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((4 * 200) + c1)]), _mm256_load_ps(&B[((4 * 200) + c1)])), _mm256_load_ps(&B[((4 * 200) + c1)])));
    _mm256_store_ps(&C[((5 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((5 * 200) + c1)]), _mm256_load_ps(&B[((5 * 200) + c1)])), _mm256_load_ps(&B[((5 * 200) + c1)])));
    _mm256_store_ps(&C[((6 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((6 * 200) + c1)]), _mm256_load_ps(&B[((6 * 200) + c1)])), _mm256_load_ps(&B[((6 * 200) + c1)])));
    _mm256_store_ps(&C[((7 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((7 * 200) + c1)]), _mm256_load_ps(&B[((7 * 200) + c1)])), _mm256_load_ps(&B[((7 * 200) + c1)])));
    _mm256_store_ps(&C[((8 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((8 * 200) + c1)]), _mm256_load_ps(&B[((8 * 200) + c1)])), _mm256_load_ps(&B[((8 * 200) + c1)])));
    _mm256_store_ps(&C[((9 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((9 * 200) + c1)]), _mm256_load_ps(&B[((9 * 200) + c1)])), _mm256_load_ps(&B[((9 * 200) + c1)])));
    _mm256_store_ps(&C[((10 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((10 * 200) + c1)]), _mm256_load_ps(&B[((10 * 200) + c1)])), _mm256_load_ps(&B[((10 * 200) + c1)])));
    _mm256_store_ps(&C[((11 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((11 * 200) + c1)]), _mm256_load_ps(&B[((11 * 200) + c1)])), _mm256_load_ps(&B[((11 * 200) + c1)])));
    _mm256_store_ps(&C[((12 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((12 * 200) + c1)]), _mm256_load_ps(&B[((12 * 200) + c1)])), _mm256_load_ps(&B[((12 * 200) + c1)])));
    _mm256_store_ps(&C[((13 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((13 * 200) + c1)]), _mm256_load_ps(&B[((13 * 200) + c1)])), _mm256_load_ps(&B[((13 * 200) + c1)])));
    _mm256_store_ps(&C[((14 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((14 * 200) + c1)]), _mm256_load_ps(&B[((14 * 200) + c1)])), _mm256_load_ps(&B[((14 * 200) + c1)])));
    _mm256_store_ps(&C[((15 * 200) + c1)], _mm256_mul_ps(_mm256_add_ps(_mm256_load_ps(&A[((15 * 200) + c1)]), _mm256_load_ps(&B[((15 * 200) + c1)])), _mm256_load_ps(&B[((15 * 200) + c1)])));
  }
)ROC";

    EXPECT_TRUE(Contains(ss.str(), Trim(target)));
  }
}

}  // namespace cinn
