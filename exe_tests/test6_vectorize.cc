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
    s0.Vectorize({4, 4});

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
  for (int c0 = 0; (c0 <= 24); c0 += 4) {
    for (int c1 = 0; (c1 <= 49); c1 += 4) {
      // vectorize - points
      if(1) {
        __m128& var2 = *(__m128*)(&C[((c0 * 200) + c1)]);
        __m128& var1 = *(__m128*)(&B[((c0 * 200) + c1)]);
        __m128& var0 = *(__m128*)(&A[((c0 * 200) + c1)]);
        var2 = _mm_mul_ps(_mm_add_ps(var0, var1), var1);
      }

      if(1) {
        __m128& var2 = *(__m128*)(&C[(((c0 + 1) * 200) + c1)]);
        __m128& var1 = *(__m128*)(&B[(((c0 + 1) * 200) + c1)]);
        __m128& var0 = *(__m128*)(&A[(((c0 + 1) * 200) + c1)]);
        var2 = _mm_mul_ps(_mm_add_ps(var0, var1), var1);
      }

      if(1) {
        __m128& var2 = *(__m128*)(&C[(((c0 + 2) * 200) + c1)]);
        __m128& var1 = *(__m128*)(&B[(((c0 + 2) * 200) + c1)]);
        __m128& var0 = *(__m128*)(&A[(((c0 + 2) * 200) + c1)]);
        var2 = _mm_mul_ps(_mm_add_ps(var0, var1), var1);
      }

      if(1) {
        __m128& var2 = *(__m128*)(&C[(((c0 + 3) * 200) + c1)]);
        __m128& var1 = *(__m128*)(&B[(((c0 + 3) * 200) + c1)]);
        __m128& var0 = *(__m128*)(&A[(((c0 + 3) * 200) + c1)]);
        var2 = _mm_mul_ps(_mm_add_ps(var0, var1), var1);
      }

    }
    for (int c1 = 52; (c1 <= 199); c1 += 4) {
      // vectorize - points
      if(1) {
        __m128& var5 = *(__m128*)(&C[((c0 * 200) + c1)]);
        __m128& var4 = *(__m128*)(&B[((c0 * 200) + c1)]);
        __m128& var3 = *(__m128*)(&A[((c0 * 200) + c1)]);
        var5 = _mm_mul_ps(_mm_add_ps(var3, var4), var4);
      }

      if(1) {
        __m128& var5 = *(__m128*)(&C[(((c0 + 1) * 200) + c1)]);
        __m128& var4 = *(__m128*)(&B[(((c0 + 1) * 200) + c1)]);
        __m128& var3 = *(__m128*)(&A[(((c0 + 1) * 200) + c1)]);
        var5 = _mm_mul_ps(_mm_add_ps(var3, var4), var4);
      }

      if(1) {
        __m128& var5 = *(__m128*)(&C[(((c0 + 2) * 200) + c1)]);
        __m128& var4 = *(__m128*)(&B[(((c0 + 2) * 200) + c1)]);
        __m128& var3 = *(__m128*)(&A[(((c0 + 2) * 200) + c1)]);
        var5 = _mm_mul_ps(_mm_add_ps(var3, var4), var4);
      }

      if(1) {
        __m128& var5 = *(__m128*)(&C[(((c0 + 3) * 200) + c1)]);
        __m128& var4 = *(__m128*)(&B[(((c0 + 3) * 200) + c1)]);
        __m128& var3 = *(__m128*)(&A[(((c0 + 3) * 200) + c1)]);
        var5 = _mm_mul_ps(_mm_add_ps(var3, var4), var4);
      }

    }
  }
  for (int c0 = 28; (c0 <= 99); c0 += 4) {
    for (int c1 = 0; (c1 <= 199); c1 += 4) {
      // vectorize - points
      if(1) {
        __m128& var8 = *(__m128*)(&C[((c0 * 200) + c1)]);
        __m128& var7 = *(__m128*)(&B[((c0 * 200) + c1)]);
        __m128& var6 = *(__m128*)(&A[((c0 * 200) + c1)]);
        var8 = _mm_mul_ps(_mm_add_ps(var6, var7), var7);
      }

      if(1) {
        __m128& var8 = *(__m128*)(&C[(((c0 + 1) * 200) + c1)]);
        __m128& var7 = *(__m128*)(&B[(((c0 + 1) * 200) + c1)]);
        __m128& var6 = *(__m128*)(&A[(((c0 + 1) * 200) + c1)]);
        var8 = _mm_mul_ps(_mm_add_ps(var6, var7), var7);
      }

      if(1) {
        __m128& var8 = *(__m128*)(&C[(((c0 + 2) * 200) + c1)]);
        __m128& var7 = *(__m128*)(&B[(((c0 + 2) * 200) + c1)]);
        __m128& var6 = *(__m128*)(&A[(((c0 + 2) * 200) + c1)]);
        var8 = _mm_mul_ps(_mm_add_ps(var6, var7), var7);
      }

      if(1) {
        __m128& var8 = *(__m128*)(&C[(((c0 + 3) * 200) + c1)]);
        __m128& var7 = *(__m128*)(&B[(((c0 + 3) * 200) + c1)]);
        __m128& var6 = *(__m128*)(&A[(((c0 + 3) * 200) + c1)]);
        var8 = _mm_mul_ps(_mm_add_ps(var6, var7), var7);
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
