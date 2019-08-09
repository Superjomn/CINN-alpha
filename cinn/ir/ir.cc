#include "cinn/ir/ir.h"
#include "ir.h"

#include <glog/logging.h>
#include <utility>

namespace cinn {
namespace ir {

Expr Add::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";

  auto x = std::make_shared<Add>();
  x->a = a;
  x->b = b;

  Expr res(x);

  return res;
}

//-------------------- Logical expressions -------------------------
Expr EQ::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<EQ>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr NE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<NE>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

size_t Var::counter_ = 0;

template <>
Parameter::Parameter(const std::string &name, int32_t val) : name_(name) {
  type_ = primitive_t::int32;
  int32_val_ = val;
}

template <>
Parameter::Parameter(const std::string &name, float val) : name_(name) {
  type_ = primitive_t::float32;
  fp32_val_ = val;
}

template <>
Parameter::Parameter(int32_t val) : name_(DefaultUniqueName()) {
  type_ = primitive_t::int32;
  int32_val_ = val;
}

template <>
Parameter::Parameter(float val) : name_(DefaultUniqueName()) {
  type_ = primitive_t::float32;
  fp32_val_ = val;
}

unsigned int Parameter::counter = 0;

}  // namespace ir
}  // namespace cinn
