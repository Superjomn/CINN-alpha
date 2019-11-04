#include "cinn/hlir/instruction_layer/conv2d_op.h"
#include "cinn/core/function.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class Conv2d : public Operator {
 public:
  Conv2d() : Operator("conv2d", HlirLayer::kInstructionWise, nullptr) { param_.set(Conv2dParam()); }

  void Resize() override {
    auto& input0 = GetInput("X");
    auto& output0 = GetOutput("Out");
    auto& the_param = param<Conv2dParam>();
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(conv2d, kInstructionWise, ::cinn::hlir::instruction_layer::Conv2d);
