#include "cinn/hlir/instruction_layer/pad_op.h"
#include <gtest/gtest.h>
#include "cinn/core/function.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/session.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

TEST(pad, test) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "pad");
  ASSERT_TRUE(op);

  Session session;
  auto *input0 = session.NewTensor("input0");
  auto *input1 = session.NewTensor("input1");

  input0->set_shape({20, 30, 40});

  op->set_session(&session);

  op->SetInput("X", "input0");
  op->SetOutput("Out", "input1");

  auto &param = op->param<PadParam>();
  param.padding.assign({{4, 4}, {4, 4}, {4, 4}});

  op->Compile();

  LOG(INFO) << "tensor.stages size " << input1->stages().size();

  Function fn("complex");
  {
    for (auto &stage : input1->stages()) {
      fn.AddStage(stage);
    }
    fn.Inputs({input1->expr()});
    fn.EndDefinition();
  }
}

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
