#include <glog/logging.h>
#include <gtest/gtest.h>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/builder.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"

using namespace cinn;        // NOLINT
using namespace cinn::hlir;  // NOLINT

TEST(test8, basic) {
  SetGlobalContext(new CINNContext);

  hlir::Session session;
  hlir::Network net("tmp", &session);

  hlir::Network2Builder net_builder(5);
  net_builder.Build(&net, &session);

  hlir::Builder builder;
  auto expr = builder.Build(&session, &net);
  builder.ToCSourceCode(expr, "exe_test8");
}
