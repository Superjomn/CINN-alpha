#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/utils/math.h"
#include "cinn/utils/timer.h"
#include "exe_tests/exe_test6.cc"

TEST(exe, test) {
  const int M = 100;
  const int N = 200;

  std::vector<cinn_float32_t> A(M * N, 1.0);
  std::vector<cinn_float32_t> B(M * N, 2.0);
  std::vector<cinn_float32_t> C(M * N, 0.f);
  std::vector<cinn_float32_t> C1(M * N, 0.f);

  cinn::RandomVec(A.data(), A.size());
  cinn::RandomVec(B.data(), B.size());

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      C[i * N + j] = (A[i * N + j] + B[i * N + j]) * B[i * N + j];
      C[i * N + j] += 1.f;
    }
  }

  basic(A.data(), B.data(), &C1[0]);

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      EXPECT_NEAR(C[i * N + j], C1[i * N + j], 1e-5);
    }
  }
}
