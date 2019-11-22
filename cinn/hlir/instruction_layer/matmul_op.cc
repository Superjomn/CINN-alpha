#include "cinn/hlir/instruction_layer/matmul_op.h"
#include <vector>
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class MatMulOp : public Operator {
 public:
  MatMulOp() : Operator("matmul", HlirLayer::kInstructionWise, nullptr) { param_.set(MatMulParam()); }

 protected:
  void InferenceOutputType() override {
    const auto* input0 = GetInput("X");
    const auto* W = GetInput("W");
    auto& output = GetOutput("Out");

    CHECK_EQ(input0->ptype(), W->ptype());
    CHECK_NE(input0->ptype(), primitive_t::unk);

    output.set_ptype(input0->ptype());
  }

  void Resize() override {
    auto* input0 = GetInput("X");
    auto* W = GetInput("W");
    auto& output0 = GetOutput("Out");

    CHECK(input0);
    CHECK(W);

    CHECK_EQ(input0->shape().size(), 2UL);
    CHECK_EQ(W->shape().size(), 2UL);
    CHECK_EQ(input0->shape()[1], W->shape()[0]);

    std::vector<int> shape({input0->shape()[0], W->shape()[1]});
    output0.set_shape(Shape(shape));
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

    ir::Expr i, j, k;
    i = input0->iterators()[0];
    k = W->iterators()[0];
    j = W->iterators()[1];

    TensorAppendExpr(&output0,  //
                     out[i][j] = Expr(0.f));

    TensorAppendExpr(&output0,  //
                     out[i][j] += x[i][k] * w[k][j]);

    input0->set_iterators({i, k});
    W->set_iterators({k, j});
    output0.set_iterators({i, j});
  }
};

//! MatMul with argument b transposed.
class MatMulTransposedOp : public Operator {
 public:
  MatMulTransposedOp() : Operator("matmul_transposed", HlirLayer::kInstructionWise, nullptr) {
    param_.set(MatMulTransposedParam());
  }

 private:
  void InferenceOutputType() override {
    const auto* input0 = GetInput("X");
    const auto* W = GetInput("W");
    auto& output = GetOutput("Out");

    CHECK_EQ(input0->ptype(), W->ptype());
    CHECK_NE(input0->ptype(), primitive_t::unk);

    output.set_ptype(input0->ptype());
  }

  void Resize() override {
    auto* input0 = GetInput("X");
    auto* W = GetInput("W");
    auto& output0 = GetOutput("Out");

    CHECK(input0);
    CHECK(W);

    CHECK_EQ(input0->shape().size(), 2UL);
    CHECK_EQ(W->shape().size(), 2UL);
    CHECK_EQ(input0->shape()[1], W->shape()[1]);

    std::vector<int> shape({input0->shape()[0], W->shape()[0]});
    output0.set_shape(Shape(shape));

    // set iterators
    ir::Expr i, j, k;
    i = input0->iterators()[0];
    k = W->iterators()[0];
    j = W->iterators()[1];

    input0->set_iterators({i, k});
    W->set_iterators({j, k});
    output0.set_iterators({i, j});
  }

  void CompileImpl() override {
    LOG_INDENT(1);
    auto* input0 = GetInput("X");
    auto* W = GetInput("W");
    auto& output0 = GetOutput("Out");

    TensorAppendExpr(&output0,  //
                     output0.Elem() = Expr(0.f));
    TensorAppendExpr(&output0,  //
                     output0.Elem() += input0->Elem() * W->Elem());
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(matmul, kInstructionWise, ::cinn::hlir::instruction_layer::MatMulOp);
REGISTER_OP(matmul_transposed, kInstructionWise, ::cinn::hlir::instruction_layer::MatMulTransposedOp);
