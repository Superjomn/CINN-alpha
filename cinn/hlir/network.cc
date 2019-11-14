#include "cinn/hlir/network.h"
#include <cinn/ir/ir.h>
#include "cinn/hlir/instruction_layer/reshape_op.h"
#include "cinn/hlir/op_registry.h"

namespace cinn {
namespace hlir {

Network::Var Network::AddMatMul(const Var &x, const Var &y) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "matmul");
  op->set_session(session_);
  op->SetInput("X", x.name);
  op->SetInput("W", y.name);

  Var out(NameGenerator::Global().NewTmpVar());
  session_->NewTensor(out.name);
  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

Network::Var Network::AddTanh(Var x) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "tanh");
  op->set_session(session_);
  op->SetInput("X", x.name);

  Var out(NameGenerator::Global().NewTmpVar());
  session_->NewTensor(out.name);
  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

Network::Var Network::AddSigmoid(Var x) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "sigmoid");
  op->set_session(session_);
  op->SetInput("X", x.name);

  Network::Var out(NameGenerator::Global().NewTmpVar());
  session_->NewTensor(out.name);
  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

Network::Var Network::AddElementwise(ElementwiseOpKind kind, Var x, Var y) {
  std::unique_ptr<Operator> op;

  switch (kind) {
    case Network::ElementwiseOpKind::kAdd:
      op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "elementwise_add");
      break;
    case Network::ElementwiseOpKind::kSub:
      op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "elementwise_sub");
      break;
    case Network::ElementwiseOpKind::kMul:
      op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "elementwise_mul");
      break;
    case Network::ElementwiseOpKind::kDiv:
      op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "elementwise_div");
      break;
  }
  op->set_session(session_);
  op->SetInput("X", x.name);
  op->SetInput("Y", y.name);

  Var out(NameGenerator::Global().NewTmpVar());
  session_->NewTensor(out.name);
  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

Network::Var Network::AddReshape(const std::vector<int> &shape, Var x) {
  instruction_layer::ReshapeParam param;
  param.shape = shape;

  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "reshape");
  op->param<instruction_layer::ReshapeParam>() = param;

  Var out(NameGenerator::Global().NewTmpVar());
  session_->NewTensor(out.name);
  op->SetInput("X", x.name);
  op->SetOutput("Out", out.name);

  operators_.emplace_back(std::move(op));
}

Program Network::Compile() {
  Program program;
  for (auto &op : operators_) {
    program.AddOp(std::move(op));
  }
  operators_.clear();
  return program;
}

Network::Var Network::AddFc(Network::Var x, Network::Var w, Network::Var b) {
  Var out = AddMatMul(x, w);
  if (b) {
    out = AddElementwise(ElementwiseOpKind::kAdd, out, b);
  }
  return out;
}

Network::Var Network::DeclInput(const std::string &name, primitive_t ptype, Shape shape) {
  input_names_.insert(name);

  Tensor *tensor = session_->NewTensor(name);
  CHECK(tensor);
  tensor->set_shape(shape);
  tensor->set_ptype(ptype);
  return Var(name);
}

Network::Var Network::DeclOutput(const std::string &name) {
  outputs_names_.insert(name);

  Tensor *tensor = session_->GetTensor(name);
  CHECK(tensor);
  return Var(name);
}

}  // namespace hlir
}  // namespace cinn
