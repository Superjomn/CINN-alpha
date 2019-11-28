#include <glog/logging.h>
#include <gtest/gtest.h>
#include <mkl.h>
#include <vector>
#include "cinn/utils/math.h"
#include "cinn/utils/timer.h"
#include "exe_tests/exe_test6.cc"

void MklRun(float* A, float* B, float* C, float* C1, int n) {
  vsMul(n, B, B, C);
  vsAdd(n, A, C, C);
  vsMul(n, B, B, C);
  vsAdd(n, A, C, C);
}

TEST(exe, test) {
  const int M = 100;
  const int N = 200;

  float* A = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* B = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* C = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));
  float* C1 = static_cast<float*>(aligned_alloc(32, M * N * sizeof(float)));

  cinn::RandomVec(A, M * N);
  cinn::RandomVec(B, M * N);
  cinn::RandomVec(C, M * N);
  cinn::RandomVec(C1, M * N);

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      C[i * N + j] = (A[i * N + j] + B[i * N + j]) * B[i * N + j];
      C[i * N + j] = (A[i * N + j] + B[i * N + j]) * B[i * N + j];
      // C[i * N + j] += 1.f;
    }
  }

  int repeat = 1000;

  {
    cinn::Timer timer;
    timer.Start();
    for (int i = 0; i < repeat; i++) MklRun(A, B, C, C1, M * N);
    timer.Stop();
    LOG(INFO) << "mkl duration: " << static_cast<double>(timer.duration()) / repeat;
  }

  {
    cinn::Timer timer;
    timer.Start();
    for (int i = 0; i < repeat; i++) basic(A, B, &C1[0]);
    timer.Stop();
    LOG(INFO) << "avx duration: " << static_cast<double>(timer.duration()) / repeat;
  }

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      // EXPECT_NEAR(C[i * N + j], C1[i * N + j], 1e-5);
    }
  }
}
