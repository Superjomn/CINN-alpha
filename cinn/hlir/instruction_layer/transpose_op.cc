#include "cinn/hlir/instruction_layer/transpose_op.h"
#include <vector>
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class TransposeOp : public Operator {
 public:
  TransposeOp() : Operator("transpose", HlirLayer::kInstructionWise, nullptr) { param_.set(TransposeParam()); }

 protected:
  void InferenceOutputType() override {
    const auto* input0 = GetInput("X");
    auto& output = GetOutput("Out");

    output.set_ptype(input0->ptype());
  }

  void Resize() override {
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");
    CHECK(input0);
    CHECK_GE(input0->shape().size(), 2UL);

    Shape out_shape = input0->shape();

    const auto& perm = param<TransposeParam>().perm;
    std::vector<ir::Expr> iterators;

    CHECK_EQ(input0->iterators().size(), perm.size());
    for (int i = 0; i < input0->shape().size(); i++) {
      out_shape[i] = input0->shape()[perm[i]];
      CHECK(input0->iterators()[perm[i]].valid());
      auto iter = input0->iterators()[perm[i]];
      iterators.push_back(iter);
    }
    output0.set_shape(out_shape);
    output0.set_iterators(iterators);
  }

  void CompileImpl() override {
    LOG_INDENT(1);
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");

    output0.AddStage(output0.Elem() = input0->Elem());
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(transpose, kInstructionWise, ::cinn::hlir::instruction_layer::TransposeOp);
