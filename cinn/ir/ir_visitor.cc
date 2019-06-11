#include "cinn/ir/ir_visitor.h"
#include "cinn/ir/expr.h"

namespace cinn {
namespace ir {

void IRVisitor::visit(const Stmt*) {}

void IRVisitor::visit(const Add* x) {
  x->a.Accept(this);
  x->b.Accept(this);
}

void IRVisitor::visit(const Sub* x) {
  x->a.Accept(this);
  x->b.Accept(this);
}

void IRVisitor::visit(const Mul* x) {
  x->a.Accept(this);
  x->b.Accept(this);
}

void IRVisitor::visit(const IntImm* x) {}
void IRVisitor::visit(const FloatImm* x) {}

}  // namespace ir
}  // namespace cinn
