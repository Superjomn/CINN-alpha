#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/hlir/network_test_util.h"
#include "cinn/utils/math.h"
#include "cinn/utils/timer.h"
#include "exe_tests/exe_test12.cc"

namespace cinn {

void FC(float* A, float* B, float* C, float* bias, int M, int N, int K) {
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      for (int k = 0; k < K; k++) {
        C[i * N + j] += A[i * K + k] * B[j * K + k];
      }

      C[i * N + j] += bias[j];
      C[i * N + j] = std::max(0.f, C[i * N + j]);
    }
  }
}

TEST(test12, basic) {
  const int M = 100;
  const int N = 200;
  const int K = 160;

  float* A = static_cast<float*>(aligned_alloc(32, M * K * sizeof(float)));
  float* B = static_cast<float*>(aligned_alloc(32, N * K * sizeof(float)));
  float* bias = static_cast<float*>(aligned_alloc(32, N * sizeof(float)));
  float* C = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* C1 = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));

  cinn::RandomVec(A, M * K);
  cinn::RandomVec(B, N * K);
  cinn::RandomVec(bias, N);

  LOG(INFO) << "C: " << C[1];
  LOG(INFO) << "C1: " << C1[1];
  memset(C, 0, M * N * sizeof(float));
  memset(C1, 0, M * N * sizeof(float));

  basic(A, B, bias, C);

  FC(A, B, C1, bias, M, N, K);

  // LOG(INFO) << C[1];

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      EXPECT_NEAR(C[i * N + j], C1[i * N + j], 1e-5);
    }
  }
}

}  // namespace cinn
