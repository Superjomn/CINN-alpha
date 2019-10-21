#pragma once
#include <list>
#include <memory>

#include "cinn/hlir/operator.h"
#include "cinn/hlir/program.h"
#include "cinn/hlir/tensor.h"
#include "cinn/utils/dot.h"

namespace cinn {
namespace hlir {

/**
 * Node the the node in a Graph, it contains an Operator and input and output links.
 */
struct Node {
  std::list<Node*> inlinks;
  std::list<Node*> outlinks;
  Operator* op{};
  Tensor* tensor{};
  std::string name;

  bool is_op() const { return op; }
  bool is_tensor() const { return tensor; }
};

static int dot_node_count = 0;

/**
 * Graph helps to transpose a Program to an SSA graph.
 */
class Graph {
 public:
  /**
   * Build a Graph from a Program.
   * @param program
   */
  void Build(const Program& program, const Session& session);

  const std::vector<std::unique_ptr<Node>>& nodes() const { return nodes_; }

  std::string dot() const;

  void NewOpNode(Operator* op);

  void NewTensorNode(const std::string& name);

 private:
  std::map<std::string, Node*> vars_;
  std::vector<std::unique_ptr<Node>> nodes_;
  const Program* program_;
  const Session* session_;
};

}  // namespace hlir
}  // namespace cinn
