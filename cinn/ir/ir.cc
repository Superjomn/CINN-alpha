#include "cinn/ir/ir.h"
#include <glog/logging.h>
#include <utility>

namespace cinn {
namespace ir {

Expr Add::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";

  auto* x = new Add;
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

Expr Sub::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";

  auto* x = new Sub;
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

Expr Mul::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Mul a not defined";
  CHECK(b.valid()) << "Mul b not defined";

  auto* x = new Mul;
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

Expr Div::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Mul a not defined";
  CHECK(b.valid()) << "Mul b not defined";

  auto* x = new Div;
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

Expr Mod::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Mul a not defined";
  CHECK(b.valid()) << "Mul b not defined";

  auto* x = new Mod;
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

//-------------------- Logical expressions -------------------------
Expr EQ::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto* node = new EQ;
  node->a = std::move(a);
  node->b = std::move(b);
  return node;
}

Expr NE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto* node = new NE;
  node->a = std::move(a);
  node->b = std::move(b);
  return node;
}

}  // namespace ir
}  // namespace cinn
