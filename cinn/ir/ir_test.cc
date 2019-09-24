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

TEST(ir, complex2) {
  Expr a(1), b(2), c;
  c = a % b + 1;
}

TEST(ir, Image) {
  Constant C("C", primitive_t::int32);
  Constant H("H", primitive_t::int32);
  Constant W("W", primitive_t::int32);
  Tensor input("image0", primitive_t::float32, {C, H, W});
}

TEST(ir, Var) {
  Constant zero("zero", primitive_t::int32);
  Constant N("N", primitive_t::int32);
  Interval interval(zero, N);

  Var i("i", primitive_t::int32, interval);
}

TEST(ir, basic1) {
  Constant C("C", 200);
  Constant H("H", 100);
  Constant W("W", 20);
  Tensor input("image0", primitive_t::float32, {C, H, W});

  Constant zero("zero", 0);
  Var i("c", primitive_t::int32, zero, C);
  Var j("j", primitive_t::int32, zero, H);
  Var k("k", primitive_t::int32, zero, W);
}

TEST(Parameter, basic) {
  Constant x(100);
  ASSERT_EQ(x.primitive_type(), primitive_t::int32);

  Constant y = x;
  ASSERT_EQ(x.As<int32_t>(), 100);
}

TEST(Interval, basic) {
  Interval interval(0, 100);
  LOG(INFO) << interval.lower_bound().As<int32_t>() << " " << interval.upper_bound().As<int32_t>();
  ASSERT_EQ(interval.lower_bound().As<int32_t>(), 0);
  ASSERT_EQ(interval.upper_bound().As<int32_t>(), 100);
}

TEST(Interval, basic1) {
  Interval interval(Constant(0), Constant(100));
  ASSERT_EQ(interval.lower_bound().As<int32_t>(), 0);
  ASSERT_EQ(interval.upper_bound().As<int32_t>(), 100);
  LOG(INFO) << interval.lower_bound().As<int32_t>() << " " << interval.upper_bound().As<int32_t>();
}

TEST(Reference, basic1) {
  Expr A("A");
  Var i("i", 0, 100);
  Var j("j", 0, 100);
  Expr r = A[i][j];

  ASSERT_TRUE(r.valid());
  ASSERT_TRUE(r.type() == NodeTy::Reference);
  auto* ref = r.As<Reference>();
  ASSERT_EQ(ref->iterators.size(), 2UL);

  ASSERT_EQ(ref->target.type(), ir::NodeTy::Var);
  ASSERT_EQ(ref->target.As<ir::Var>()->name(), "A");

  auto& it0 = *ref->iterators[0].As<Var>();
  ASSERT_EQ(it0.name(), "i");
  ASSERT_EQ(it0.interval().lower_bound().As<int32_t>(), 0);
  ASSERT_EQ(it0.interval().upper_bound().As<int32_t>(), 100);

  auto& it1 = *ref->iterators[1].As<Var>();
  ASSERT_EQ(it1.name(), "j");
  ASSERT_EQ(it1.interval().lower_bound().As<int32_t>(), 0);
  ASSERT_EQ(it1.interval().upper_bound().As<int32_t>(), 100);

  auto interval0 = it0.interval();
  auto interval1 = it1.interval();

  ASSERT_EQ(interval0.lower_bound().As<int32_t>(), 0);
  ASSERT_EQ(interval0.upper_bound().As<int32_t>(), 100);
}

TEST(Param, basic) {
  Param x("N", "N > 0");
  LOG(INFO) << x.GetContext();

  std::stringstream ss;
  ss << x.GetContext();

  ASSERT_EQ(ss.str(), "[N] -> {  : N > 0 }");
}

}  // namespace ir
}  // namespace cinn
