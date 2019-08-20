#include "cinn/ir/ir.h"
#include "ir.h"

#include <glog/logging.h>
#include <utility>

namespace cinn {
namespace ir {

Expr Add::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<Add>();
  node->a = a;
  node->b = b;
  return Expr(node);
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

Expr For::make(Expr min, Expr extent, Expr body) {
  CHECK(min.valid());
  CHECK(extent.valid());
  CHECK(body.valid());
  auto node = std::make_shared<For>();
  node->min = std::move(min);
  node->extent = std::move(extent);
  node->body = std::move(body);
  return Expr(node);
}

Expr Mod::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Mod>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Sub::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Mod>();
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

Expr Mul::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Mul a not defined";
  CHECK(b.valid()) << "Mul b not defined";

  auto node = std::make_shared<Mul>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Div::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Div a not defined";
  CHECK(b.valid()) << "Div b not defined";

  auto x = std::make_shared<Div>();
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

Expr LT::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<LT>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr LE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<LE>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr GT::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<GT>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr GE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<GE>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

std::set<std::string> Var::name_set_;

Expr Block::make(const std::vector<Expr> &list) {
  for (auto &v : list) {
    CHECK(v.valid());
  }
  auto node = std::make_shared<Block>();
  node->list = list;
  return Expr(node);
}

}  // namespace ir
}  // namespace cinn
