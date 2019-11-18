#include <gtest/gtest.h>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/builder.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network.h"
#include "cinn/hlir/network_test_util.h"

namespace cinn {

TEST(test7, basic) {
  SetGlobalContext(new CINNContext);

  hlir::Session session;
  hlir::Network net("tmp", &session);

  hlir::Network1Builder net_builder;
  net_builder.Build(&net, &session);

  hlir::Builder builder;
  auto expr = builder.Build(&session, &net);
  builder.ToCSourceCode(expr, "exe_test7");
}

}  // namespace cinn
