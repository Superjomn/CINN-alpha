#include "cinn/ir/ir_visitor.h"
#include "cinn/ir/expr.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/macros.h"
#include "ir_visitor.h"

namespace cinn {
namespace ir {

#define OP_2_ARGS_VISIT(op__)             \
  void IRVisitor::Visit(const op__ *op) { \
    CHECK(op->a.valid());                 \
    CHECK(op->b.valid());                 \
    op->a.Accept(this);                   \
    op->b.Accept(this);                   \
  }

OP_2_ARGS_FOR_EACH(OP_2_ARGS_VISIT);

void IRVisitor::Visit(const Expr *op) {
  switch (op->type()) {
#define __(op__)                  \
  case NodeTy::op__:              \
    op->As<op__>()->Accept(this); \
    break;

    OP_2_ARGS_FOR_EACH(__);
  }
}
void IRVisitor::Visit(const Stmt *op) {}

void IRVisitor::Visit(const Minus *op) { op->a.Accept(this); }

void IRVisitor::Visit(const IntImm *op) {}
void IRVisitor::Visit(const FloatImm *op) {}

void IRVisitor::Visit(const Tensor *op) {}
void IRVisitor::Visit(const Var *op) {}
void IRVisitor::Visit(const Parameter *op) {}
void IRVisitor::Visit(const For *op) {}
void IRVisitor::Visit(const Block *op) {}

}  // namespace ir
}  // namespace cinn
