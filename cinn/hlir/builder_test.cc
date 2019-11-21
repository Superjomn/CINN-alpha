#include "cinn/hlir/builder.h"
#include <gtest/gtest.h>
#include "cinn/backends/code_gen_c.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"

namespace cinn {
namespace hlir {

TEST(builder, weight) {
  SetGlobalContext(new CINNContext);

  Session session;
  Network net("tmp", &session);
  Network1Builder net_builder;
  net_builder.Build(&net, &session);

  Builder builder;
  auto expr = builder.Build(&session, &net);

  backends::C_CodeGen gen;
  gen.Print(expr);

  auto program = gen.compiled_code();
  LOG(INFO) << std::endl << program << std::endl;

  std::string target = R"ROC(// create weight buffers
cinn_float32_t b[] = {0.100000,0.200000};
cinn_float32_t w0[] = {0.100000,0.200000,0.300000,0.400000,0.500000,0.600000,0.700000,0.800000};
// create input buffers
cinn_float32_t* x0 =  (cinn_float32_t*) malloc(48);
// create output buffers
cinn_float32_t* tmp1 =  (cinn_float32_t*) malloc(24);
// create temporary variable buffers
cinn_float32_t* tmp0 =  (cinn_float32_t*) malloc(24);
cinn_float32_t* tmp2 =  (cinn_float32_t*) malloc(24);

// functions for reading output data
void get_output_tmp1 (cinn_float32_t* tmp1_) {
  cinn_copy(tmp1, tmp1_, 24);
}
// functions for loadding input data
void set_input_x0 (cinn_float32_t* x0_) {
  cinn_copy(x0_, x0, 48);
}
void func9 (cinn_float32_t* b, cinn_float32_t* w0, cinn_float32_t* x0, cinn_float32_t* tmp2) {
  for (int c0 = 0; (c0 <= 2); c0 += 1) {
    for (int c1 = 0; (c1 <= 1); c1 += 1) {
      tmp0[c0, c1] = 0;
      for (int c2 = 0; (c2 <= 3); c2 += 1) {
        tmp0[c0, c1] += (x0[c0, c2] * w0[c2, c1]);
      }
      tmp1[c0, c1] = (tmp0[c0, c1] + b[c1]);
      tmp2[c0, c1] = cinn_max(tmp1[c0, c1], 0);
    }
  }
}
void main_ () {
  func9(b, w0, x0, tmp2);
})ROC";

  ASSERT_EQ(program, target);
}

}  // namespace hlir
}  // namespace cinn
