#include "cinn/hlir/graph.h"
#include <algorithm>
#include "cinn/core/function.h"
#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ir_visitor.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace hlir {
namespace optimize {

/**
 * Transform Graph to IR functions.
 */
class GraphToIrFunctionsPass : public Pass<Graph> {
 public:
  explicit GraphToIrFunctionsPass(const std::string& name) : Pass(name) {}

 protected:
  void Impl(Graph* graph) {
    LOG_INDENT(1);
    for (auto& node : GraphTraits::TS(*graph)) {
      if (node.is_tensor()) {
        CINN_DEBUG(2) << node.tensor->name() << " " << node.tensor->__repr__();
        TensorSpreadOneStep(&node);
        CINN_DEBUG(1) << node.tensor->__repr__();
        CINN_DEBUG(1) << "node.expr: " << ir::Dump(node.tensor->expr());
        CINN_DEBUG(2) << "outlink.size " << node.outlinks.size();
        CINN_DEBUG(2) << "stage size: " << node.tensor->stages().size();
        CINN_DEBUG(2) << "spreaded stage size: " << tensor_accumulated_stages_[node.tensor->name()].size();
      }
    }

    auto funcs = GenerateFunctions(graph);
    LOG(INFO) << "funcs: " << funcs.size();
    graph->arguments().SetFunctions(std::move(funcs));
  }

  /**
   * Go one step forward.
   * @param tensor_node
   */
  void TensorSpreadOneStep(Node* tensor_node) {
    CHECK(tensor_node->is_tensor());

    // concat the stages of the current tensor node to accumulated_stages.
    auto& accumulated_stages = GetAccumulatedStages(tensor_node);
    for (auto& stage : tensor_node->tensor->stages()) {
      accumulated_stages.push_back(stage);
    }

    // if this node is a endpoint, that means a ir::Function will be generated and make this tensor a input.
    // so it shouldn't spread.
    if (tensor_node->IsEndPoint()) return;

    for (auto& out_op : tensor_node->outlinks) {
      for (auto& out_tensor : out_op->outlinks) {
        auto& accumulated_stages = GetAccumulatedStages(out_tensor);
        for (auto& stage : tensor_node->tensor->stages()) {
          accumulated_stages.push_back(stage);
        }
      }
    }
  }

  std::vector<Function> GenerateFunctions(Graph* graph) {
    std::vector<Function> result;

    for (auto* out_node : graph->Outputs()) {
      Function _func(NameGenerator::Global().NewFuncionName());
      result.emplace_back(_func);
      auto& func = result.back();

      auto& stages = GetAccumulatedStages(out_node);
      for (auto& stage : stages) {
        func.AddStage(stage);
      }

      std::vector<ir::Expr> fn_inputs, fn_outputs;
      CHECK(!func.stages().empty());
      ExtractIOsFromStages(graph, func.stages(), &fn_inputs, &fn_outputs);
      func.Inputs(fn_inputs);
      func.Outputs(fn_outputs);

      func.EndDefinition();

      LOG(INFO) << "func.inputs: " << func.inputs().size();
      LOG(INFO) << "func.outputs: " << func.outputs().size();

      LOG(INFO) << "func:\n" << ir::Dump(func.ir_function());
    }
    return result;
  }

  void ExtractIOsFromStages(Graph* graph,
                            const std::vector<Stage>& stages,
                            std::vector<Expr>* fn_inputs,
                            std::vector<Expr>* fn_outputs) {
    class Collector : public ir::IRVisitor {
     public:
      std::set<std::string> tensors_read;
      std::set<std::string> tensors_write;

      void Visit(const Expr* op) override { IRVisitor::Visit(op); }

      void Visit(const ir::Assign* op) override {
        is_left_branch = true;
        Visit(&op->a);
        is_left_branch = false;
        Visit(&op->b);
      }
      void Visit(const ir::SumAssign* op) override {
        LOG(INFO) << "visiting SumAssign";
        is_left_branch = true;
        Visit(&op->a);
        is_left_branch = false;
        Visit(&op->b);
      }
      void Visit(const ir::SubAssign* op) override {
        is_left_branch = true;
        Visit(&op->a);
        is_left_branch = false;
        Visit(&op->b);
      }
      void Visit(const ir::MulAssign* op) override {
        is_left_branch = true;
        Visit(&op->a);
        is_left_branch = false;
        Visit(&op->b);
      }
      void Visit(const ir::DivAssign* op) override {
        is_left_branch = true;
        Visit(&op->a);
        is_left_branch = false;
        Visit(&op->b);
      }
      void Visit(const ir::Tensor* op) override {
        LOG(INFO) << "visiting a Tensor";
        if (is_left_branch) {
          tensors_write.insert(op->name());
        } else {
          tensors_read.insert(op->name());
        }
      }

      bool is_left_branch = false;
    };

    Collector collector;
    for (auto& stage : stages) {
      LOG(INFO) << "collecting expr " << ir::Dump(stage.expr());
      collector.Visit(&stage.expr());
    }

    CHECK(!collector.tensors_write.empty());
    CHECK(!collector.tensors_read.empty());

    std::set<std::string> inputs, outputs;
    auto get_tensor_expr = [&](const std::string& ir_inner_name) {
      auto it = std::find_if(graph->nodes().begin(), graph->nodes().end(), [&](const std::unique_ptr<Node>& node) {
        return node->is_tensor() && node->tensor->ir_inner_name() == ir_inner_name;
      });
      CHECK(it != graph->nodes().end()) << "can't find a node whose ir_inner_name is " << ir_inner_name;
      return (*it)->tensor->expr();
    };

    for (auto& ir_inner_name : collector.tensors_read) {
      fn_inputs->push_back(get_tensor_expr(ir_inner_name));
    }

    for (auto& ir_inner_name : collector.tensors_write) {
      if (!collector.tensors_write.count(ir_inner_name)) fn_outputs->push_back(get_tensor_expr(ir_inner_name));
    }
  }

  std::vector<Stage>& GetAccumulatedStages(const Node* x) { return tensor_accumulated_stages_[x->tensor->name()]; }

 private:
  //! The stages that accumulate until this tensor.
  std::map<std::string /*tensor name*/, std::vector<Stage>> tensor_accumulated_stages_;
};

}  // namespace optimize
}  // namespace hlir
}  // namespace cinn

REGISTER_HLIR_PASS(graph_to_ir_functions, ::cinn::hlir::optimize::GraphToIrFunctionsPass);
