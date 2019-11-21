#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include "cinn/hlir/network_test_util.h"
#include "cinn/utils/timer.h"
#include "exe_tests/exe_test10.cc"

TEST(exe, test) {
  cinn::hlir::Network2Builder builder(2, false);

  std::vector<float> input(builder.x0_shape.num_elements(), 1.0);
  std::vector<float> output(10 * builder.dim, 0.);

  set_input_x0(input.data());

  int repeat = 1000;
  cinn::Timer timer;
  timer.Start();
  for (int i = 0; i < repeat; i++) {
    main_();
  }
  timer.Stop();
  LOG(INFO) << "duration: " << static_cast<float>(timer.duration()) / repeat;

  get_output_tmp7(&output[0]);

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
