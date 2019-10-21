#include "cinn/hlir/graph.h"
#include <gtest/gtest.h>
#include <utility>
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/op_registry.h"

namespace cinn {
namespace hlir {

/*
 *   inputs: x0, x1, w0
 */
void BuildProgram(Session* session, Program* program) {
  auto x0 = session->NewTensor("x0");
  auto y0 = session->NewTensor("y0");
  auto y1 = session->NewTensor("y1");
  auto w0 = session->NewTensor("w0");
  auto w1 = session->NewTensor("w1");

  x0->set_shape({20, 30});
  w0->set_shape({30, 40});
  w1->set_shape({40, 50});

  auto matmul0 = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "matmul");
  matmul0->SetInput("X", "x0");
  matmul0->SetInput("W", "w0");
  matmul0->SetOutput("Out", "y0");  // {20, 40}

  auto matmul1 = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "matmul");
  matmul1->SetInput("X", "y0");
  matmul1->SetInput("W", "w1");
  matmul1->SetOutput("Out", "y1");  // {20, 50}

  program->AddOp(std::move(matmul0));
  program->AddOp(std::move(matmul1));
}

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
