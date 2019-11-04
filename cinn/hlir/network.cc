#include "cinn/hlir/network.h"
#include "cinn/hlir/instruction_layer/reshape_op.h"
#include "cinn/hlir/op_registry.h"

namespace cinn {
namespace hlir {

void Network::AddMatMul(const std::string &x, const std::string &y, const std::string &out) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "matmul");
  op->set_session(session_);
  op->SetInput("X", x);
  op->SetInput("W", y);
  op->SetOutput("Out", out);
  operators_.emplace_back(std::move(op));
}

void Network::AddTanh(const std::string &x, const std::string &out) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "tanh");
  op->set_session(session_);
  op->SetInput("X", x);
  op->SetOutput("Out", out);
  operators_.emplace_back(std::move(op));
}

void Network::AddSigmoid(const std::string &x, const std::string &out) {
  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "sigmoid");
  op->set_session(session_);
  op->SetInput("X", x);
  op->SetOutput("Out", out);
  operators_.emplace_back(std::move(op));
}

void Network::AddElementwise(Network::ElementwiseOpKind kind,
                             const std::string &x,
                             const std::string &y,
                             const std::string &out) {
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
    default:
      LOG(FATAL) << "Not Implemented";
  }

  op->SetInput("X", x);
  op->SetInput("Y", y);
  op->SetOutput("Out", out);
  op->set_session(session_);
  operators_.emplace_back(std::move(op));
}

void Network::AddReshape(const std::vector<int> &shape, const std::string &x, const std::string &out) {
  instruction_layer::ReshapeParam param;
  param.shape = shape;

  auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "reshape");
  op->param<instruction_layer::ReshapeParam>() = param;

  op->SetInput("X", x);
  op->SetOutput("Out", out);

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

}  // namespace hlir
}  // namespace cinn
