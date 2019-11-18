#include "cinn/hlir/network.h"
#include <gtest/gtest.h>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"

namespace cinn {
namespace hlir {

TEST(network, basic) {
  Session session;
  Network net("tmp", &session);

  Network1Builder net_builder;
  net_builder.Build(&net, &session);

  auto program = net.Compile();

  Graph graph;
  graph.Build(program, session);

  for (Node& node : GraphTraits::TS(graph)) {
    LOG(INFO) << "visiting node " << node.name;
    if (node.is_op()) node.op->Compile();
  }

  LOG(INFO) << "DOT:\n" << graph.dot();

  graph.Compile(false);
}

}  // namespace hlir
}  // namespace cinn
