#include "cinn/hlir/builder.h"
#include "cinn/ir/ir_helper.h"

namespace cinn {
namespace hlir {

ir::Expr Builder::Build(Session *session, Network &&net) {
  Program program = net.Compile();

  Graph graph;
  graph.Build(program, *session);

  for (Node &node : GraphTraits::TS(graph)) {
    if (node.is_op()) node.op->Compile();
  }

  graph.PartitionFunctions();

  LOG(INFO) << "DOT:\n" << graph.dot();

  auto main_expr = graph.CompileExpr();
  auto global_vars = CreateGlobalVars(session, net);

  auto expr = ir::Module::make(global_vars, main_expr);

  LOG(INFO) << "program:\n" << expr;

  return expr;
}

Expr Builder::CreateExprForWeightDeclaration(const Session &session, const Network &network) {
  std::vector<ir::Expr> exprs;

  for (auto &x : network.weight_names()) {
    Tensor *tensor = session.GetTensor(x);
    CHECK(tensor);
    Target target;
    CHECK(tensor->ptype() != primitive_t::unk);
    Expr size(tensor->shape().num_bytes(tensor->ptype()));
    auto expr = ir::BufferOpr::make(target, size, ir::BufferOpr::Opr::kCreateAssign, tensor->ptype(), tensor->name());
    expr.As<ir::BufferOpr>()->assigned_data.set<std::vector<float>>(tensor->buffer()->data<std::vector<float>>());
    exprs.push_back(expr);
  }

  ir::Expr block = ir::Block::make(std::move(exprs));
  return block;
}

Expr Builder::CreateExprForInputOutputDeclaration(const Session &session, const Network &network) {
  std::vector<ir::Expr> exprs;

  auto create_tensor = [&](const std::string &name) {
    auto *tensor = session.GetTensor(name);
    CHECK(tensor);
    CHECK(tensor->ptype() != primitive_t::unk);
    Expr size(tensor->shape().num_bytes(tensor->ptype()));
    Target target;
    auto expr = ir::BufferOpr::make(target, size, ir::BufferOpr::Opr::kCreate, tensor->ptype(), tensor->name());
    exprs.emplace_back(expr);
  };

  for (auto &x : network.input_names()) {
    create_tensor(x);
  }
  for (auto &x : network.output_names()) {
    create_tensor(x);
  }

  return ir::Block::make(std::move(exprs));
}

}  // namespace hlir
}  // namespace cinn
