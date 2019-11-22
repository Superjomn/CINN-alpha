#include <glog/logging.h>
#include <gtest/gtest.h>
#include <fstream>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/builder.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"

using namespace cinn;        // NOLINT
using namespace cinn::hlir;  // NOLINT

TEST(test8, basic) {
  SetGlobalContext(new CINNContext);

  hlir::Session session;
  hlir::Network net("tmp", &session);

  hlir::Network2Builder net_builder(2, true);
  net_builder.Build(&net, &session);

  hlir::Builder builder;
  auto expr = builder.Build(&session, &net);
  builder.ToCSourceCode(expr, "exe_test10");

  {
    std::fstream file("exe_test10.cc");
    CHECK(file.is_open());
    std::string line;
    std::stringstream ss;
    while (std::getline(file, line)) {
      ss << line << '\n';
    }
    file.close();

    std::string target = R"ROC(// functions for reading output data
void get_output_tmp7 (cinn_float32_t* tmp7_) {
  cinn_copy(tmp7, tmp7_, 5120);
}
// functions for loadding input data
void set_input_x0 (cinn_float32_t* x0_) {
  cinn_copy(x0_, x0, 5120);
}
void func15 (cinn_float32_t* b, cinn_float32_t* w0, cinn_float32_t* x0, cinn_float32_t* tmp7) {
  // call once statement
  if(tmp8) {
    for (int c0 = 0; (c0 <= 127); c0 += 1) {
      for (int c1 = 0; (c1 <= 127); c1 += 1) {
        tmp0[((c0 * 128) + c1)] = w0[((c1 * 128) + c0)];
      }
    }
    tmp8 = 0;
  }

}
void func16 (cinn_float32_t* b, cinn_float32_t* w0, cinn_float32_t* x0, cinn_float32_t* tmp7) {
  // call once statement
  if(tmp9) {
    for (int c0 = 0; (c0 <= 127); c0 += 1) {
      for (int c1 = 0; (c1 <= 127); c1 += 1) {
        tmp4[((c0 * 128) + c1)] = w0[((c1 * 128) + c0)];
      }
    }
    tmp9 = 0;
  }

}
void func17 (cinn_float32_t* b, cinn_float32_t* w0, cinn_float32_t* x0, cinn_float32_t* tmp7) {
  for (int c0 = 0; (c0 <= 9); c0 += 1) {
    for (int c1 = 0; (c1 <= 127); c1 += 1) {
      tmp1[((c0 * 128) + c1)] = 0;
      for (int c2 = 0; (c2 <= 127); c2 += 1) {
        tmp1[((c0 * 128) + c1)] += (x0[((c0 * 128) + c2)] * tmp0[((c1 * 128) + c2)]);
      }
      tmp2[((c0 * 128) + c1)] = (tmp1[((c0 * 128) + c1)] + b[c1]);
      tmp3[((c0 * 128) + c1)] = cinn_max(tmp2[((c0 * 128) + c1)], 0);
    }
  }
  for (int c0 = 0; (c0 <= 9); c0 += 1) {
    for (int c1 = 0; (c1 <= 127); c1 += 1) {
      tmp5[((c0 * 128) + c1)] = 0;
      for (int c2 = 0; (c2 <= 127); c2 += 1) {
        tmp5[((c0 * 128) + c1)] += (tmp3[((c0 * 128) + c2)] * tmp4[((c1 * 128) + c2)]);
      }
      tmp6[((c0 * 128) + c1)] = (tmp5[((c0 * 128) + c1)] + b[c1]);
      tmp7[((c0 * 128) + c1)] = cinn_max(tmp6[((c0 * 128) + c1)], 0);
    }
  }
}
void main_ () {
  func15(b, w0, x0, tmp7);
  func16(b, w0, x0, tmp7);
  func17(b, w0, x0, tmp7);
})ROC";

    std::string code_gen = ss.str();
    ASSERT_TRUE(code_gen.find(target) != std::string::npos);
  }
}
