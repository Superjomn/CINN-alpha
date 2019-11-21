#include "cinn/hlir/network.h"
#include "cinn/hlir/instruction_layer/reshape_op.h"
#include "cinn/hlir/instruction_layer/transpose_op.h"
#include "cinn/hlir/op_registry.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {

Network::Var Network::AddMatMul(const Var &x, const Var &y, bool y_transposed) {
  std::string op_type = y_transposed ? "matmul_transposed" : "matmul";
  auto matmul_op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, op_type);

  Network::Var transpose_out = y;
  if (y_transposed) {
    // Add transpose op
    auto *y_tensor = session_->GetTensor(y.name);
    CHECK_EQ(y_tensor->shape().size(), 2UL);
    transpose_out = AddTranspose(y, {1, 0});
  }

  matmul_op->set_session(session_);
  matmul_op->SetInput("X", x.name);
  matmul_op->SetInput("W", transpose_out.name);

  Var out(GlobalContext().name_generator().NewTmpVar());
  DeclTmpVar(out.name);
  matmul_op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(matmul_op));
  return out;
}

Network::Var Network::AddTanh(Var x) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "tanh");
  op->set_session(session_);
  op->SetInput("X", x.name);

  Var out(GlobalContext().name_generator().NewTmpVar());
  DeclTmpVar(out.name);
  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

Network::Var Network::AddSigmoid(Var x) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "sigmoid");
  op->set_session(session_);
  op->SetInput("X", x.name);

  Network::Var out(GlobalContext().name_generator().NewTmpVar());
  DeclTmpVar(out.name);
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

  Var out(GlobalContext().name_generator().NewTmpVar());
  DeclTmpVar(out.name);
  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

Network::Var Network::AddReshape(const std::vector<int> &shape, Var x) {
  instruction_layer::ReshapeParam param;
  param.shape = shape;

  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "reshape");
  op->param<instruction_layer::ReshapeParam>() = param;

  Var out(GlobalContext().name_generator().NewTmpVar());
  DeclTmpVar(out.name);
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

Network::Var Network::AddFc(Network::Var x, Network::Var w, Network::Var b, bool w_transposed) {
  Var out = AddMatMul(x, w, w_transposed);
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
  CHECK(is_tmp_var(name));
  tmp_var_names_.erase(name);
  output_names_.insert(name);

  Tensor *tensor = session_->GetTensor(name);
  CHECK(tensor);
  return Var(name);
}

Tensor *Network::DeclTmpVar(const std::string &name) {
  CHECK(IsVarNameAvailable(name)) << "name '" << name << "' is duplicate";
  tmp_var_names_.insert(name);
  return session_->NewTensor(name);
}

bool Network::IsVarNameAvailable(const std::string &name) const {
  return !(is_input(name) || is_output(name) || is_weight(name) || is_tmp_var(name));
}

Network::Var Network::AddTranspose(const Network::Var &x, const std::vector<int> &perm) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "transpose");
  op->set_session(session_);
  op->SetInput("X", x.name);
  op->param<hlir::instruction_layer::TransposeParam>().perm = perm;

  Var out(GlobalContext().name_generator().NewTmpVar());
  DeclTmpVar(out.name);

  op->SetOutput("Out", out.name);
  operators_.emplace_back(std::move(op));
  return out;
}

}  // namespace hlir
}  // namespace cinn
