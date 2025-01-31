#include "cinn/hlir/graph.h"
#include <gtest/gtest.h>
#include <utility>
#include "cinn/hlir/graph_test_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/op_registry.h"

namespace cinn {
namespace hlir {

TEST(Graph, build) {
  Session session;
  Program program;

  BuildProgram(&session, &program);

  Graph graph;
  graph.Build(program, session);

  LOG(INFO) << graph.dot();
}

}  // namespace hlir
}  // namespace cinn
