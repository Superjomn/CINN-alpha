#include <gtest/gtest.h>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/builder.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network.h"
#include "cinn/hlir/network_test_util.h"

namespace cinn {

TEST(test7, basic) {
  hlir::Session session;
  hlir::Network net("tmp", &session);

  hlir::BuildNetwork1(&net, &session);

  hlir::Builder builder;
  auto expr = builder.Build(&session, std::move(net));
  builder.ToCSourceCode(expr, "exe_test7");
}

}  // namespace cinn
