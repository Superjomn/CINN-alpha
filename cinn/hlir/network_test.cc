#include "cinn/hlir/network.h"
#include <gtest/gtest.h>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"

namespace cinn {
namespace hlir {

TEST(network, basic) {
  Session session;

  Network net("tmp", &session);
  net.AddMatMul("x0", "w", "y0");
  net.AddElementwise(Network::ElementwiseOpKind::kAdd, "y0", "b", "y1");
  net.AddTanh("y1", "y2");

  auto* x0 = session.NewTensor("x0");
  x0->set_shape({30, 40});

  auto* w = session.NewTensor("w");
  w->set_shape({40, 50});

  auto* b = session.NewTensor("b");
  b->set_shape({50});

  auto* y0 = session.NewTensor("y0");
  auto* y1 = session.NewTensor("y1");
  auto* y2 = session.NewTensor("y2");

  auto program = net.Compile();

  Graph graph;
  graph.Build(program, session);

  for (Node& node : GraphTraits::TS(graph)) {
    LOG(INFO) << "visiting node " << node.name;
    if (node.is_op()) node.op->Compile();
  }

  LOG(INFO) << "DOT:\n" << graph.dot();

  graph.Compile();
}

}  // namespace hlir
}  // namespace cinn