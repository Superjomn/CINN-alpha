#include "cinn/ir/ir.h"
#include <gtest/gtest.h>
#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace ir {

TEST(ir, basic) {
  Expr a(1), b(2);

  ASSERT_EQ(a.As<IntImm>()->val(), 1);
  ASSERT_EQ(a.As<IntImm>()->type().bits(), 32);

  ASSERT_EQ(b.As<IntImm>()->val(), 2);
  ASSERT_EQ(b.As<IntImm>()->type().bits(), 32);

  Expr c = a + b;

  auto* cc = c.As<Add>();
  ASSERT_TRUE(cc);
  ASSERT_EQ(cc->a.As<IntImm>()->val(), 1);
  ASSERT_EQ(cc->b.As<IntImm>()->val(), 2);
}

TEST(ir, complex1) {
  Expr a(1), b(2), c, d(3);
  c = (a + b) * d;
}

}  // namespace ir
}  // namespace cinn
