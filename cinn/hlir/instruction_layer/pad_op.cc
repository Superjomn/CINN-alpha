#include "cinn/hlir/instruction_layer/pad_op.h"
#include "cinn/core/function.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class Pad : public Operator {
 public:
  Pad() : Operator("pad", HlirLayer::kInstructionWise, nullptr) { param_.set(PadParam()); }

 protected:
  void Resize() override {
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");
    auto& the_param = param<PadParam>();

    std::vector<int> ir_shape;
    for (int i = 0; i < input0->shape().size(); i++) {
      ir_shape.emplace_back(input0->shape()[i] + the_param.padding[i][0].int32_val() +
                            the_param.padding[i][1].int32_val());
    }

    output0.set_shape(ir_shape);
  }

 protected:
  void CompileImpl() override {
    LOG_INDENT(0);
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");
    auto& the_param = param<PadParam>();
    CHECK(!output0.shape().empty());

    CHECK_EQ(input0->shape().size(), output0.shape().size());
    CHECK_EQ(output0.iterators().size(), the_param.padding.size());

    // pad in each dimension
    for (int i = 0; i < output0.iterators().size(); i++) {
      CHECK_EQ(the_param.padding[i].size(), 2UL);
      const ir::Constant& pre_padding = the_param.padding[i][0];
      const ir::Constant& post_padding = the_param.padding[i][1];

      ir::Var pre_iter(NameGenerator::Global().NewNamed("i"), primitive_t::int32);

      CINN_DEBUG(3) << "output0.elem: " << ir::Dump(output0.Elem());
      output0.AddStage(output0.Elem() = Expr(0.f));
      output0.last_stage().SetCond(output0.iterators()[i], StringFormat(" < %d", pre_padding.int32_val()));

      const auto& s0 = output0.AddStage(output0.Elem() = Expr(0.f));
      CINN_DEBUG(1) << "stage iterator domain: " << s0.iterator_domain();
      output0.last_stage().SetCond(output0.iterators()[i],
                                   StringFormat(" >= %d", output0.shape()[i] - post_padding.int32_val()));
    }
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(pad, kInstructionWise, ::cinn::hlir::instruction_layer::Pad);
