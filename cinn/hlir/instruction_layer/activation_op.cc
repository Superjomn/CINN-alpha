#include "cinn/core/function.h"
#include "cinn/hlir/operator.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

class Tanh : public Operator {
 protected:
  void CompileImpl() override {
    auto& input0 = GetInput("X");
    auto& output0 = GetOutput("Out");

    CHECK_EQ(input0.shape().size(), output0.shape().size());
    output0.AddStage(  //
        output0.Elem().Assign(Max_(input0.Elem(), Expr(0.f))));
  }
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
