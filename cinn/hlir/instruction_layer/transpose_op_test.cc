#include "cinn/hlir/instruction_layer/transpose_op.h"
#include <gtest/gtest.h>
#include <string>
#include "cinn/backends/code_gen_c.h"
#include "cinn/core/function.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/ir/ir_helper.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

TEST(matmul_op, test) {
  SetGlobalContext(new CINNContext);

  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "transpose");
  ASSERT_TRUE(op);

  op->param<TransposeParam>().perm = {0, 2, 3, 1};

  Session session;
  auto *input0 = session.NewTensor("x");
  auto *output = session.NewTensor("out");

  input0->set_ptype(primitive_t::float32);
  output->set_ptype(primitive_t::float32);

  input0->set_shape({20, 30, 40, 50});

  op->set_session(&session);

  op->SetInput("X", "x");
  op->SetOutput("Out", "out");

  op->Compile();

  Function fn("complex");
  {
    for (auto &stage : output->stages()) {
      fn.AddStage(stage);
    }
    fn.Inputs({input0->expr()});
    fn.Outputs({output->expr()});
    fn.EndDefinition();
  }

  backends::C_CodeGen gen;
  gen.Print(fn.ir_function());

  LOG(INFO) << "generated code:\n" << gen.compiled_code();

  std::string target = R"ROC(void complex (cinn_float32_t* x, cinn_float32_t* out) {
  for (int c0 = 0; (c0 <= 19); c0 += 1) {
    for (int c1 = 0; (c1 <= 39); c1 += 1) {
      for (int c2 = 0; (c2 <= 49); c2 += 1) {
        for (int c3 = 0; (c3 <= 29); c3 += 1) {
          out[c0, c1, c2, c3] = x[c0, c3, c1, c2];
        }
      }
    }
  }
})ROC";

  ASSERT_EQ(gen.compiled_code(), target);
}

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
