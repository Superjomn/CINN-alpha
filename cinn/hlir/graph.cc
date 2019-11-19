#include "cinn/hlir/graph.h"
#include <algorithm>
#include "cinn/backends/code_gen_c.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {

static int node_count = 0;

void Graph::Build(const Program& program, const Session& session) {
  program_ = &program;
  session_ = &session;

  LOG_INDENT(0);
  for (auto& x : program.Inputs()) {
    CINN_DEBUG(2) << "add input " << x << " to program";
    NewTensorNode(x);
  }

  for (auto& op : program.ops()) {
    NewOpNode(op.get());
  }
}

std::string Graph::dot() const {
  Dot dot;
  CHECK(!nodes().empty());

  for (auto& x : vars_) {
    dot.AddNode(x.first, {});
  }

  for (auto& node : nodes()) {
    if (node->is_tensor()) {
      // dot.AddNode(node->name, {});

      for (auto& x : node->inlinks) {
        dot.AddEdge(x->name, node->name, {});
      }
    } else {  // op
      dot.AddNode(node->name, {});

      for (auto& x : node->inlinks) {
        dot.AddEdge(x->name, node->name, {});
      }
    }
  }

  return dot.Build();
}

void Graph::NewOpNode(Operator* op) {
  LOG_INDENT(6);
  nodes_.emplace_back(new Node);
  auto* op_node = nodes_.back().get();
  op_node->name = op->type() + std::to_string(node_count++);
  op_node->op = op;

  // link inputs
  for (auto& item : op->inputs()) {
    CHECK(vars_.count(item.second));
    auto* var_node = vars_[item.second];
    var_node->outlinks.push_back(op_node);
    op_node->inlinks.push_back(var_node);
    CINN_DEBUG(2) << "in " << var_node->name << " -> " << op_node->name;
  }

  // link outputs
  for (auto& item : op->outputs()) {
    NewTensorNode(item.second);
    auto* var_node = vars_[item.second];
    var_node->inlinks.push_back(op_node);
    op_node->outlinks.push_back(var_node);
    CINN_DEBUG(2) << "out " << op_node->name << " -> " << var_node->name;
  }
}

void Graph::NewTensorNode(const std::string& name) {
  LOG_INDENT(6);
  CINN_DEBUG(2) << "new tensor " << name;
  CHECK(!vars_.count(name));
  nodes_.emplace_back(new Node);
  auto* var_node = nodes_.back().get();
  var_node->name = name;
  var_node->tensor = session_->GetTensor(name);

  vars_[name] = var_node;
}

std::set<const Node*> Graph::Inputs() const {
  std::set<const Node*> result;
  for (auto& node : nodes()) {
    if (node->is_tensor() && node->inlinks.empty()) {
      result.insert(node.get());
    }
  }
  return result;
}

std::set<const Node*> Graph::Outputs() const {
  std::set<const Node*> result;
  for (auto& node : nodes()) {
    if (node->is_tensor() && node->outlinks.empty()) {
      result.insert(node.get());
    }
  }
  return result;
}

std::set<Node*> Graph::Inputs() {
  std::set<Node*> result;
  for (auto& node : nodes()) {
    if (node->is_tensor() && node->inlinks.empty()) {
      result.insert(node.get());
    }
  }
  return result;
}

std::set<Node*> Graph::Outputs() {
  std::set<Node*> result;
  for (auto& node : nodes()) {
    if (node->is_tensor() && node->outlinks.empty()) {
      result.insert(node.get());
    }
  }
  return result;
}

Node* Graph::GetTensor(const std::string& name) {
  auto it = vars_.find(name);
  CHECK(it != vars_.end());
  return it->second;
}

std::vector<ir::Expr> SortInputsOrOutputs(const std::set<Node*>& xs) {
  std::vector<ir::Expr> ys;
  std::transform(xs.begin(), xs.end(), std::back_inserter(ys), [](Node* node) { return node->tensor->expr(); });

  auto expr_compare = [](const ir::Expr& a, const ir::Expr& b) {
    auto a_str = ir::Dump(a);
    auto b_str = ir::Dump(b);
    return a_str < b_str;
  };

  std::sort(ys.begin(), ys.end(), expr_compare);
  return ys;
}

std::vector<Function> Graph::PartitionFunctions() {
  LOG_INDENT(0);

  std::vector<ir::Expr> fn_inputs = SortInputsOrOutputs(Inputs());
  std::vector<ir::Expr> fn_outputs = SortInputsOrOutputs(Outputs());

  CINN_DEBUG(1) << "inputs.size " << fn_inputs.size();
  CINN_DEBUG(1) << "outputs.size " << fn_outputs.size();

  // the cache for accumulated stages of op.
  std::map<Node* /*op*/, std::vector<Stage>> op_accu_stages;

  /*
   *    T0    T1
   *      \   /
   *       Op0    Op1
   *         \    /
   *           T2
   * Op will collect all the input tensors' stages.
   * Tensor will also collect all the input Ops' stages.
   */
  auto collect_tensor_input_stages = [&](Node* tensor_node) {
    CHECK(tensor_node->tensor);
    for (Node* in_op_node : tensor_node->inlinks) {
      CHECK(op_accu_stages.count(in_op_node));
      for (auto& stage : op_accu_stages[in_op_node]) {
        tensor_node->tensor->AddStage(stage);
      }
    }
  };

  auto collect_op_input_stages = [&](Node* op_node) {
    CHECK(op_node->op);
    for (Node* in_tensor_node : op_node->inlinks) {
      if (in_tensor_node->outlinks.size() > 1) continue;
      op_accu_stages[op_node].insert(op_accu_stages[op_node].end(),
                                     in_tensor_node->tensor->stages().begin(),
                                     in_tensor_node->tensor->stages().end());
    }
  };

  // Accumulate the stages in operators and tensors.
  for (Node& node : GraphTraits::TS(*this)) {
    if (node.tensor)
      collect_tensor_input_stages(&node);
    else
      collect_op_input_stages(&node);
  }

  // Partition the functions. We make a tensor a function(composed of all the stages in the tensor) if
  // 1. The tensor node has more than one outlinks(multiple consumers)
  // 2. The tensor node is a output node of the whole graph.

  std::set<Node*> in_nodes = Inputs();
  std::set<Node*> out_nodes = Outputs();
  std::vector<Function> fns;

  for (Node& node : GraphTraits::TS(*this)) {
    if (!node.is_tensor()) continue;
    auto* tensor = node.tensor;

    if (in_nodes.count(&node)) continue;  // skip the input nodes of the graph.
    if (node.outlinks.size() > 1 || out_nodes.count(&node)) {
      fns.emplace_back(GlobalContext().name_generator().NewFuncionName());
      Function& fn = fns.back();
      CINN_DEBUG(2) << "partition a new function " << fn.name();
      CHECK(!tensor->stages().empty()) << "tensor " << tensor->name() << " has no stages";
      for (auto& stage : tensor->stages()) {
        fn.AddStage(stage);
        CINN_DEBUG(2) << "collect stage: " << stage.expr();
        fn.Inputs(fn_inputs);
        fn.Outputs(fn_outputs);
      }
      // fn.EndDefinition();
    }
  }

  return fns;
}

void Graph::Compile(bool finalize_function) {
  // check all the operators are compiled
  for (Node& node : GraphTraits::DFS(*this)) {
    auto* op = node.op;
    if (op) CHECK(op->compiled());
  }

  AllocateBuffersForTempVars();

  auto fns = PartitionFunctions();

  CHECK(!fns.empty());
  for (auto& fn : fns) {
    fn.EndDefinition();
  }

  std::vector<ir::Expr> exprs;
  std::transform(fns.begin(), fns.end(), std::back_inserter(exprs), [](const Function& x) { return x.ir_function(); });
  auto block = ir::Block::make(std::move(exprs));

  backends::CompileAsC(block, "1.h", "1.cc");
}

Expr Graph::CompileExpr(std::vector<Function>* fns) {
  LOG_INDENT(0);
  CINN_DEBUG(2) << "fns.size " << fns->size();
  CHECK(!fns->empty());
  for (auto& fn : *fns) {
    fn.EndDefinition();
  }

  AllocateBuffersForTempVars();

  std::vector<ir::Expr> exprs;
  std::transform(
      fns->begin(), fns->end(), std::back_inserter(exprs), [](const Function& x) { return x.ir_function(); });
  auto block = ir::Block::make(std::move(exprs));
  return block;
}

void Graph::AllocateBuffersForTempVars() {
  for (Node& node : GraphTraits::DFS(*this)) {
    auto* tensor = node.tensor;
    if (tensor) {
      CHECK(tensor->ptype() != primitive_t::unk);
      if (!tensor->buffer()) {
        tensor->AttachBuffer();
      }
    }
  }
}

}  // namespace hlir
}  // namespace cinn
