#pragma once
#include <list>
#include <memory>

#include "cinn/hlir/operator.h"
#include "cinn/hlir/tensor.h"

namespace cinn {
namespace hlir {

/**
 * Node the the node in a Graph, it contains an Operator and input and output links.
 */
class Node {
  std::list<Node*> inlinks;
  std::list<Node*> outlinks;
  std::weak_ptr<Operator> op;
  std::weak_ptr<Tensor> tensor;
};

/**
 * Graph helps to transpose a Program to an SSA graph.
 */
class Graph {
 public:
  void NewOpNode(const std::shared_ptr<Operator>& op);

  void NewTensorNode();

 private:
};

}  // namespace hlir
}  // namespace cinn
