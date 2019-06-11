#include "cinn/ir/ir.h"
#include <glog/logging.h>

namespace cinn {
namespace ir {

Expr Add::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto* node = new Add;
  node->a = std::move(a);
  node->b = std::move(b);
  return node;
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
