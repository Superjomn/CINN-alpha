#include "cinn/hlir/builder.h"
#include <gtest/gtest.h>
#include "cinn/backends/code_gen_c.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"

namespace cinn {
namespace hlir {

TEST(builder, weight) {
  NameGenerator::Global().ResetCounter();

  Session session;
  Network net("tmp", &session);
  BuildNetwork1(&net, &session);

  Builder builder;
  auto expr = builder.Build(&session, std::move(net));

  backends::C_CodeGen gen;
  gen.Print(expr);

  auto program = gen.compiled_code();
  LOG(INFO) << "\n" << program;

  std::string target = R"ROC(cinn_float32_t b[] = {0.100000,0.200000};
cinn_float32_t w0[] = {0.100000,0.200000,0.300000,0.400000,0.500000,0.600000,0.700000,0.800000};
cinn_float32_t* x0 =  (cinn_float32_t*) malloc(48);
cinn_float32_t* tmp1 =  (cinn_float32_t*) malloc(24);

void func9 (cinn_float32_t* b_2, cinn_float32_t* w0_1, cinn_float32_t* x0_0, cinn_float32_t* tmp2_5) {
  for (int c0 = 0; (c0 <= 2); c0 += 1) {
    for (int c1 = 0; (c1 <= 1); c1 += 1) {
      for (int c2 = 0; (c2 <= 3); c2 += 1) {
        tmp0_3[c0, c1] += (x0_0[c0, c2] * w0_1[c2, c1]);
      }
    }
  }
  for (int c0 = 0; (c0 <= 2); c0 += 1) {
    for (int c1 = 0; (c1 <= 1); c1 += 1) {
      tmp1_4[c0, c1] = (tmp0_3[c0, c1] + b_2[c1]);
    }
  }
  for (int c0 = 0; (c0 <= 2); c0 += 1) {
    for (int c1 = 0; (c1 <= 1); c1 += 1) {
      tmp2_5[c0, c1] = max(tmp1_4[c0, c1],0);
    }
  }
})ROC";

  ASSERT_EQ(program, target);
}

}  // namespace hlir
}  // namespace cinn
