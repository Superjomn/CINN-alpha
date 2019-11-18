#include <gtest/gtest.h>
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_test_util.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/optimize/optimizer.h"
#include "cinn/hlir/optimize/use_passes.h"
#include "cinn/ir/ir_printer.h"

namespace cinn {
namespace hlir {
namespace optimize {

TEST(hlir_optimizer, basic) {
  SetGlobalContext(new CINNContext);

  Session session;
  Program program;
  BuildProgram(&session, &program);

  Graph graph;
  graph.Build(program, session);

  RunProgram(&program);

  HlirOptimizer optimizer;
  optimizer(&graph);

  auto &funcs = graph.arguments().functions();
  ASSERT_EQ(funcs.size(), 1UL);
  auto generated = ir::Dump(funcs.front().ir_function());

  std::string target = R"ROC(def func8 (Tensor& w0, Tensor& w1, Tensor& x0, Tensor& y0) {
  for(c0, 0, (c0 <= 19), 1) {
    for(c1, 0, (c1 <= 39), 1) {
      for(c2, 0, (c2 <= 29), 1) {
        y0<20,40>[c0,c1] += (x0<20,30>[c0,c2] * w0<30,40>[c2,c1]);
      }
    }
  }
  for(c0, 0, (c0 <= 19), 1) {
    for(c1, 0, (c1 <= 49), 1) {
      for(c2, 0, (c2 <= 39), 1) {
        y1<20,50>[c0,c1] += (y0<20,40>[c0,c2] * w1<40,50>[c2,c1]);
      }
    }
  }
})ROC";

  ASSERT_EQ(generated, target);
}

}  // namespace optimize
}  // namespace hlir
}  // namespace cinn
