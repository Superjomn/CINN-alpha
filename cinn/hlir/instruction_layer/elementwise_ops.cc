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

  void InferenceOutputType() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Out");

    CHECK_EQ(x->ptype(), y->ptype());
    CHECK_NE(x->ptype(), primitive_t::unk);

    output.set_ptype(x->ptype());
  }

 protected:
  void Resize() override {
    const auto* x = GetInput("X");
    const auto* y = GetInput("Y");
    auto& output = GetOutput("Out");

    output.set_shape(x->shape());
    // check the reversed dimensions matches.
    CHECK_LE(y->shape().size(), x->shape().size());
    auto y_it = y->shape().data.rbegin();
    auto x_it = x->shape().data.rbegin();
    for (int i = 0; i < y->shape().size(); i++) {
      CHECK_EQ(*(y_it + i), *(x_it + i));
    }

    // share the iterators.
    y->set_iterators(std::vector<ir::Expr>(x->iterators().begin() + x->iterators().size() - y->iterators().size(),
                                           x->iterators().end()));
    output.set_iterators(x->iterators());
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
