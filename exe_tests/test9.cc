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
    for (int c2 = 0; (c2 <= 15); c2 += 1) {
      __m256& var2 = *(__m256*)(&C[((c2 * 200) + c1)]);
      __m256& var1 = *(__m256*)(&B[((c2 * 200) + c1)]);
      __m256& var0 = *(__m256*)(&A[((c2 * 200) + c1)]);
      var2 = _mm256_mul_ps(_mm256_add_ps(var0, var1), var1);
    }
  }
  for (int c1 = 32; (c1 <= 199); c1 += 8) {
    // vectorize - points
    for (int c2 = 0; (c2 <= 15); c2 += 1) {
      __m256& var5 = *(__m256*)(&C[((c2 * 200) + c1)]);
      __m256& var4 = *(__m256*)(&B[((c2 * 200) + c1)]);
      __m256& var3 = *(__m256*)(&A[((c2 * 200) + c1)]);
      var5 = _mm256_mul_ps(_mm256_add_ps(var3, var4), var4);
    }
  }
  for (int c0 = 16; (c0 <= 99); c0 += 16) {
    for (int c1 = 0; (c1 <= 199); c1 += 8) {
      // vectorize - points
      for (int c2 = 0; (c2 <= cinn_min(15, ((-c0) + 99))); c2 += 1) {
        __m256& var8 = *(__m256*)(&C[(((c0 + c2) * 200) + c1)]);
        __m256& var7 = *(__m256*)(&B[(((c0 + c2) * 200) + c1)]);
        __m256& var6 = *(__m256*)(&A[(((c0 + c2) * 200) + c1)]);
        var8 = _mm256_mul_ps(_mm256_add_ps(var6, var7), var7);
      }
    }
  }
  for (int c0 = 0; (c0 <= 99); c0 += 1) {
    for (int c1 = 0; (c1 <= 199); c1 += 1) {
      C[((c0 * 200) + c1)] += 1;
    }
  }
}
)ROC";

    EXPECT_TRUE(Contains(ss.str(), Trim(target)));
  }
}

}  // namespace cinn
