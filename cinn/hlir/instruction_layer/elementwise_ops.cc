#include "cinn/hlir/instruction_layer/elementwise_ops.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/hlir/operator.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class ElementwiseBase : public Operator {
 public:
  ElementwiseBase(const std::string& type) : Operator(type, HlirLayer::kInstructionWise, nullptr) {
    param_.set(ElementwiseParam());
  }

 protected:
  void Resize() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Out");

    output.set_shape(x->shape());
  }
};

class ElementwiseAdd : public ElementwiseBase {
 public:
  ElementwiseAdd() : ElementwiseBase("elementwise_add") {}

 protected:
  void CompileImpl() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Out");

    output.AddStage(  //
        output.Elem() = x->Elem() + y->Elem());
  }
};

class ElementwiseSub : public ElementwiseBase {
 public:
  ElementwiseSub() : ElementwiseBase("elementwise_sub") {}

 protected:
  void CompileImpl() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Y");

    output.AddStage(  //
        output.Elem() = x->Elem() - y->Elem());
  }
};

class ElementwiseMul : public ElementwiseBase {
 public:
  ElementwiseMul() : ElementwiseBase("elementwise_mul") {}

 protected:
  void CompileImpl() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Y");

    output.AddStage(  //
        output.Elem() = x->Elem() * y->Elem());
  }
};

class ElementwiseDiv : public ElementwiseBase {
 public:
  ElementwiseDiv() : ElementwiseBase("elementwise_div") {}

 protected:
  void CompileImpl() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Y");

    output.AddStage(  //
        output.Elem() = x->Elem() / y->Elem());
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn

REGISTER_OP(elementwise_add, kInstructionWise, ::cinn::hlir::instruction_layer::ElementwiseAdd);
REGISTER_OP(elementwise_sub, kInstructionWise, ::cinn::hlir::instruction_layer::ElementwiseSub);
REGISTER_OP(elementwise_div, kInstructionWise, ::cinn::hlir::instruction_layer::ElementwiseDiv);
REGISTER_OP(elementwise_mul, kInstructionWise, ::cinn::hlir::instruction_layer::ElementwiseMul);
