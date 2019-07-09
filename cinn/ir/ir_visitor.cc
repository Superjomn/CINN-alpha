#include "cinn/ir/ir_visitor.h"
#include "cinn/ir/expr.h"
#include "cinn/ir/ir.h"
#include "ir_visitor.h"

namespace cinn {
namespace ir {

void IRVisitor::Visit(const Expr *op) {}
void IRVisitor::Visit(const Stmt *op) {}
void IRVisitor::Visit(const Add *op) {
  CHECK(op->a.valid());
  CHECK(op->b.valid());
  op->a.Accept(this);
  op->b.Accept(this);
}

void IRVisitor::Visit(const Sub *op) {
  CHECK(op->a.valid());
  CHECK(op->b.valid());
  op->a.Accept(this);
  op->b.Accept(this);
}

void IRVisitor::Visit(const Mul *op) {
  CHECK(op->a.valid());
  CHECK(op->b.valid());
  op->a.Accept(this);
  op->b.Accept(this);
}

void IRVisitor::Visit(const Div *op) {
  CHECK(op->a.valid());
  CHECK(op->b.valid());
  op->a.Accept(this);
  op->b.Accept(this);
}

void IRVisitor::Visit(const IntImm *op) {}
void IRVisitor::Visit(const FloatImm *op) {}
void IRVisitor::Visit(const Var *op) {}
void IRVisitor::Visit(const NE *op) {}
void IRVisitor::Visit(const EQ *op) {}

}  // namespace ir
}  // namespace cinn
