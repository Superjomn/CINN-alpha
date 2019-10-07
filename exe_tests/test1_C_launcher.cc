#include <glog/logging.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "cinn/utils/math.h"
#include "exe_tests/exe_test1.h"

TEST(exe_tests, test1) {
  const int M = 100;
  const int N = 200;
  const int K = 150;

  std::vector<cinn_float32_t> A(M * K, 0.12), B(K * N, 0.2), C(M * N, 0);
  cinn::RandomVec(A.data(), A.size());
  cinn::RandomVec(B.data(), B.size());

  mat_mul(A.data(), B.data(), C.data());

  for (int i = 0; i < 10; i++) {
    LOG(INFO) << C[i];
  }

  // check the result
  std::vector<cinn_float32_t> C1(M * N, 0);

  for (int m = 0; m < M; m++) {
    for (int n = 0; n < N; n++) {
      for (int k = 0; k < K; k++) {
        // C[m][n] += A[m][k] * B[k][n]
        C1[m * N + n] += A[m * K + k] * B[k * N + n];
      }
      EXPECT_NEAR(C[m * N + n], C1[m * N + n], 1e-5);
    }
  }
}
