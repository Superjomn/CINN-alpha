#include "cinn/hlir/graph_util.h"
#include <gtest/gtest.h>
#include "cinn/hlir/graph_test_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"

namespace cinn {
namespace hlir {

TEST(graph_util, test) {
  Session session;
  Program program;
  BuildProgram(&session, &program);

  Graph graph;
  graph.Build(program, session);

  for (auto& node : GraphTraits::TS(graph)) {
    LOG(INFO) << "visiting node " << node.name;
  }
}

}  // namespace hlir
}  // namespace cinn
