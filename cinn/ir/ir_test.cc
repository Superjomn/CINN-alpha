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

TEST(ir, Image) {
  Parameter C("C", ScalarT::int32);
  Parameter H("H", ScalarT::int32);
  Parameter W("W", ScalarT::int32);
  Tensor input("image0", ScalarT::float32, {C, H, W});
}

TEST(ir, Var) {
  Parameter zero("zero", ScalarT::int32);
  Parameter N("N", ScalarT::int32);
  Interval interval(zero, N);

  Var i("i", ScalarT::int32, interval);
}

TEST(ir, basic1) {
  Parameter C("C", 200);
  Parameter H("H", 100);
  Parameter W("W", 20);
  Tensor input("image0", ScalarT::float32, {C, H, W});

  Parameter zero("zero", 0);
  Var i("c", ScalarT::int32, zero, C);
  Var j("j", ScalarT::int32, zero, H);
  Var k("k", ScalarT::int32, zero, W);
  Expr expr0 = input(i, j, k);
  expr0 = (expr0 + 0.5f) / 2.f;

  ASSERT_TRUE(expr0.valid());
  LOG(INFO) << static_cast<int>(expr0.type());

  // Expr expr0 = input(i, j, k) + 0.5f;
}

}  // namespace ir
}  // namespace cinn
