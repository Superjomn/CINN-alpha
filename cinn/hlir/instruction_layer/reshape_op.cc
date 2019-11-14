#include "cinn/hlir/instruction_layer/reshape_op.h"
#include "cinn/core/function.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class Reshape : public Operator {
 public:
  Reshape() : Operator("reshape", HlirLayer::kInstructionWise, nullptr) {}

 protected:
  void InferenceOutputType() override {
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");

    CHECK_NE(input0->ptype(), primitive_t::unk);
    output0.set_ptype(input0->ptype());
  }

  void Resize() override {
    auto& output0 = GetOutput("Out");
    auto& the_param = param<ReshapeParam>();
    output0.set_shape(Shape(the_param.shape));
  }

  void CompileImpl() override {
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");
    // TODO(Superjomn) check the shape.
    input0->ShareBufferWith(&output0);
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(reshape, kInstructionWise, ::cinn::hlir::instruction_layer::Reshape);
