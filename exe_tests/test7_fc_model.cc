#include <gtest/gtest.h>
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

TEST(test7, basic) {
  Session session;
  Network net("tmp", &session);
}

}  // namespace hlir
}  // namespace cinn
