#include "cinn/hlir/builder.h"
#include <gtest/gtest.h>
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network_test_util.h"

namespace cinn {
namespace hlir {

TEST(builder, test) {
  Session session;
  Network network("tmp", &session);
  BuildNetwork0(&network, &session);
  Builder builder;
  builder.Build(&session, std::move(network));
}

}  // namespace hlir
}  // namespace cinn
