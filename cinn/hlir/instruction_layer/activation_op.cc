#include "cinn/core/function.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class ActivationBase : public Operator {
 public:
  ActivationBase(const std::string& type) : Operator(type, HlirLayer::kInstructionWise, nullptr) {}

 protected:
  void Resize() override {
    const auto* x = GetInput("X");
    auto& out = GetOutput("Out");
    out.set_shape(x->shape());
    out.set_iterators(x->iterators());
  }
};

class Tanh : public ActivationBase {
 public:
  Tanh() : ActivationBase("tanh") {}

 protected:
  void CompileImpl() override {
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");

    CHECK_EQ(input0->shape().size(), output0.shape().size());
    output0.AddStage(  //
        output0.Elem().Assign(Max_(input0->Elem(), Expr(0.f))));
  }
};

class Sigmoid : public ActivationBase {
 public:
  Sigmoid() : ActivationBase("sigmoid") {}

 protected:
  void CompileImpl() override {
    auto* input0 = GetInput("X");
    auto& output0 = GetOutput("Out");
    output0.AddStage(output0.Elem().Assign(Expr(1.f) / (Expr(1.f) + Exp_(input0->Elem() * -1.f))));
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(tanh, kInstructionWise, ::cinn::hlir::instruction_layer::Tanh);
REGISTER_OP(sigmoid, kInstructionWise, ::cinn::hlir::instruction_layer::Sigmoid);
