#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/utils/math.h"
#include "cinn/utils/timer.h"
#include "exe_tests/exe_test3.cc"

TEST(exe_tests, test1) {
  const int M = 512;
  const int N = 512;
  const int K = 256;

  std::vector<cinn_float32_t> A(M * K, 0.12), B(K * N, 0.2), C(M * N, 0), Bias(N, 0);
  cinn::RandomVec(A.data(), A.size());
  cinn::RandomVec(B.data(), B.size());
  cinn::RandomVec(Bias.data(), Bias.size());

  cinn::Timer timer;
  const int repeat = 50;
  timer.Start();
  for (int i = 0; i < repeat; i++) {
    complex(A.data(), B.data(), Bias.data(), C.data());
  }
  timer.Stop();
  LOG(INFO) << "time " << timer.duration() * 1. / repeat;

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
      C1[m * N + n] += Bias[n];
      C1[m * N + n] *= 1.3f;
      LOG_FIRST_N(INFO, 30) << "C -> C1 " << C[m * N + n] << " " << C1[m * N + n];
      EXPECT_NEAR(C[m * N + n], C1[m * N + n], 1e-5);
    }
  }
}
