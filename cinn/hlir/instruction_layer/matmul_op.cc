#include "cinn/hlir/instruction_layer/matmul_op.h"
#include <vector>
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class MatMulOp : public Operator {
 public:
  MatMulOp() : Operator("matmul", HlirLayer::kInstructionWise, nullptr) { param_.set(MatMulParam()); }

 protected:
  void Resize() override {
    auto* input0 = GetInput("X");
    auto* W = GetInput("W");
    auto& output0 = GetOutput("Out");

    CHECK_EQ(input0->shape().size(), 2UL);
    CHECK_EQ(W->shape().size(), 2UL);

    std::vector<int> shape({input0->shape()[0], W->shape()[1]});
    output0.set_shape(shape);
  }

  void CompileImpl() override {
    LOG_INDENT(1);
    auto* input0 = GetInput("X");
    auto* W = GetInput("W");
    auto& output0 = GetOutput("Out");

    ir::Expr x = input0->expr(), w = W->expr(), out = output0.expr();
    CINN_DEBUG(2) << "x.expr: " << ir::Dump(x);
    CINN_DEBUG(2) << "w.expr: " << ir::Dump(w);
    CINN_DEBUG(2) << "out.expr: " << ir::Dump(out);

    ir::Var i, j, k;
    output0.AddStage(out[i][j] += x[i][k] * w[k][j]);
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(matmul, kInstructionWise, ::cinn::hlir::instruction_layer::MatMulOp);
