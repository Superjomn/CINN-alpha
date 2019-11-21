#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/utils/math.h"
#include "cinn/utils/timer.h"
#include "exe_tests/exe_test9.cc"

TEST(exe, test) {
  const int M = 100;
  const int N = 200;

  float* A = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* B = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* C = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* C1 = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));

  cinn::RandomVec(A, M * N);
  cinn::RandomVec(B, M * N);
  memset(C, 0, M * N * sizeof(float));
  memset(C1, 0, M * N * sizeof(float));

  basic(A, B, C1);

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      C[i * N + j] = (A[i * N + j] + B[i * N + j]) * B[i * N + j];
      C[i * N + j] += 1.f;
    }
  }

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      EXPECT_NEAR(C[i * N + j], C1[i * N + j], 1e-5);
    }
  }
}
