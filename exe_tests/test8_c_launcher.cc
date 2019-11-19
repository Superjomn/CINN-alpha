#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/hlir/network_test_util.h"
#include "exe_tests/exe_test8.cc"

TEST(exe, test) {
  cinn::hlir::Network2Builder builder(5);

  std::vector<float> input(builder.x0_shape.num_elements(), 1.0);
  std::vector<float> output(10 * 64, 0.);

  set_input_x0(input.data());
  main_();
  get_output_tmp14(&output[0]);

  // check result
  auto cal_result = [&](const std::vector<float>& input) {
    CHECK_EQ(builder.x0_shape[1], builder.w0_shape[0]);

    return builder.ManualTest(input);
  };

  auto output1 = cal_result(input);

  ASSERT_EQ(output.size(), output1.size());
  for (int i = 0; i < output.size(); i++) {
    EXPECT_NEAR(output[i], output1[i], 1e-5);
  }
}
