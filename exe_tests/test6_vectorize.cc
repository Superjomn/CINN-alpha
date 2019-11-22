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
    s0.Tile({16});
    s0.Vectorize(4);

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

    std::string target = R"ROC(#ifndef CINN_FILE_
#define CINN_FILE_
#include <immintrin.h>
#include <math.h>
#include <stdio.h>

typedef bool cinn_boolean_t;
typedef char cinn_int8_t;
typedef int cinn_int32_t;
typedef long long cinn_int64_t;
typedef unsigned char cinn_uint8_t;
typedef unsigned int cinn_uint32_t;
typedef unsigned long long cinn_uint64_t;
typedef float cinn_float32_t;

#define cinn_min(a,b) ((a)<(b) ? (a) : (b))
#define cinn_max(a,b) ((a)>(b) ? (a) : (b))
#define cinn_copy(a,b,size) memcpy((b), (a), (size))


void basic (cinn_float32_t* A, cinn_float32_t* B, cinn_float32_t* C) {
  // tile - tiles
  // vectorize - tiles
  for (int c0 = 0; (c0 <= 99); c0 += 1) {
    for (int c1 = 0; (c1 <= 199); c1 += 16) {
      // vectorize - points
      // tile - points
      // vectorize - tiles
      for (int c5 = 0; (c5 <= cinn_min(15, ((-c1) + 199))); c5 += 4) {
        // vectorize - points
        __m128& var2 = *(__m128*)(&C[((c0 * 200) + (c1 + c5))]);
        __m128& var1 = *(__m128*)(&B[((c0 * 200) + (c1 + c5))]);
        __m128& var0 = *(__m128*)(&A[((c0 * 200) + (c1 + c5))]);
        var2 = _mm_mul_ps(_mm_add_ps(var0, var1), var1);
      }
    }
  }
  for (int c0 = 0; (c0 <= 99); c0 += 1) {
    for (int c1 = 0; (c1 <= 199); c1 += 1) {
      C[((c0 * 200) + c1)] += 1;
    }
  }
}

#endif  // CINN_FILE_
)ROC";

    EXPECT_EQ(ss.str(), target);
  }
}

}  // namespace cinn
