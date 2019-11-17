#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/hlir/network_test_util.h"
#include "exe_tests/exe_test7.cc"

TEST(exe, test) {
  cinn::hlir::Network1Builder builder;

  std::vector<float> input(builder.x0_shape.num_elements(), 1.0);
  std::vector<float> output(6, 0.);

  set_input_x0(input.data());
  main_();
  get_output_tmp1(&output[0]);

  for (auto v : output) {
    LOG(INFO) << v;
  }

  // check result
  auto cal_result = [&builder](const std::vector<float>& input) {
    CHECK_EQ(builder.x0_shape[1], builder.w0_shape[0]);
    const int M = builder.x0_shape[0];
    const int N = builder.w0_shape[1];
    const int K = builder.w0_shape[0];

    std::vector<float> output(6, 0);
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        for (int k = 0; k < K; k++) {
          // output[i][j] += input[i][k] * w[k][j]
          output[i * N + j] += input[i * K + k] * builder.w0_data[k * N + j];
        }
        output[i * N + j] += builder.b_data[j];
        output[i * N + j] = std::max(output[i * N + j], 0.f);
      }
    }

    return output;
  };

  auto output1 = cal_result(input);

  ASSERT_EQ(output.size(), output1.size());
  for (int i = 0; i < output.size(); i++) {
    LOG(INFO) << "out" << i << " " << output[i] << " -> " << output1[i];
    EXPECT_NEAR(output[i], output1[i], 1e-5);
  }
}
