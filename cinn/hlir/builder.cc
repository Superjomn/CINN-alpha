#include "cinn/hlir/builder.h"

namespace cinn {
namespace hlir {

void Builder::Build(Session *session, Network &&net) {
  Program program = net.Compile();

  Graph graph;
  graph.Build(program, *session);

  for (Node &node : GraphTraits::TS(graph)) {
    if (node.is_op()) node.op->Compile();
  }

  graph.PartitionFunctions();

  LOG(INFO) << "DOT:\n" << graph.dot();

  graph.Compile(false);
}

}  // namespace hlir
}  // namespace cinn
