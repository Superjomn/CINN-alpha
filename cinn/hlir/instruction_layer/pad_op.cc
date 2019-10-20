#include "cinn/hlir/instruction_layer/pad_op.h"
#include "cinn/core/function.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class Pad : public Operator {
 public:
  Pad() : Operator("pad", HlirLayer::kInstructionWise, nullptr) {}

 protected:
  void CompileImpl() override {
    auto& input0 = GetInput("X");
    auto& output0 = GetOutput("Out");

    CHECK_EQ(input0.shape().size(), output0.shape().size());
    CHECK_EQ(output0.iterators().size(), param_.padding.size());

    // pad in each dimension
    for (int i = 0; i < output0.iterators().size(); i++) {
      CHECK_EQ(param_.padding[i].size(), 2UL);
      const ir::Constant& pre_padding = param_.padding[i][0];
      const ir::Constant& post_padding = param_.padding[i][1];

      ir::Var pre_iter(NameGenerator::Global().NewNamed("i"), primitive_t::int32);

      output0.AddStage(output0.Elem() = Expr(0.f));
      output0.last_stage().SetCond(output0.iterators()[i], StringFormat(" < %d", pre_padding.int32_val()));

      output0.AddStage(output0.Elem() = Expr(0.f));
      output0.last_stage().SetCond(output0.iterators()[i],
                                   StringFormat(" >= %d", output0.shape()[i] - post_padding.int32_val()));
    }
  }

  PadParam param_;
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(pad, kInstructionWise, ::cinn::hlir::instruction_layer::Pad);
