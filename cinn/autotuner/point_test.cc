#include "cinn/autotuner/point.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <utility>
#include "cinn/backends/code_gen_c.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"
#include "cinn/hlir/session.h"

namespace cinn {
namespace autotuner {

TEST(Point, basic) {
  hlir::Session session;
  hlir::Network net("tmp", &session);

  hlir::BuildNetwork0(&net, &session);

  auto program = net.Compile();

  hlir::Graph graph;
  graph.Build(program, session);

  for (hlir::Node& node : hlir::GraphTraits::TS(graph)) {
    LOG(INFO) << "visiting node " << node.name;
    if (node.is_op()) node.op->Compile();
  }

  auto fns = graph.PartitionFunctions();

  ASSERT_EQ(fns.size(), 1UL);
  ASSERT_EQ(fns.front().stages().size(), 3UL);

  Point point(&fns);
  point.Fuse(fns.front().stages()[0].name(), fns.front().stages()[1].name());
  point.Fuse(fns.front().stages()[1].name(), fns.front().stages()[2].name());
  point.BuildFns();

  std::vector<ir::Expr> exprs;
  std::transform(fns.begin(), fns.end(), std::back_inserter(exprs), [](const Function& x) { return x.ir_function(); });
  auto block = ir::Block::make(std::move(exprs));

  backends::C_CodeGen gen;
  gen.Print(block);
  auto generated_code = gen.compiled_code();
  LOG(INFO) << "compiled code:\n" << generated_code;

  std::string target =
      R"ROC(void func8 (cinn_float32_t* x00, cinn_float32_t* b2, cinn_float32_t* w1, cinn_float32_t* y25) {
  for (int c0 = 0; (c0 <= 29); c0 += 1) {
    for (int c1 = 0; (c1 <= 49); c1 += 1) {
      for (int c2 = 0; (c2 <= 39); c2 += 1) {
        y03[c0, c1] += (x00[c0, c2] * w1[c2, c1]);
      }
      y14[c0, c1] = (y03[c0, c1] + b2[c1]);
      y25[c0, c1] = max(y14[c0, c1],0);
    }
  }
})ROC";

  EXPECT_EQ(generated_code, target);
}

}  // namespace autotuner
}  // namespace cinn
