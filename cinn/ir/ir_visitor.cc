#include "cinn/ir/ir_visitor.h"
#include "cinn/core/function.h"
#include "cinn/ir/expr.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/macros.h"

namespace cinn {
namespace ir {

#define OP_2_ARGS_VISIT(op__)             \
  void IRVisitor::Visit(const op__ *op) { \
    CHECK(op->a.valid());                 \
    CHECK(op->b.valid());                 \
    Visit(&op->a);                        \
    Visit(&op->b);                        \
  };

OP_2_ARGS_FOR_EACH(OP_2_ARGS_VISIT);

void IRVisitor::Visit(const Expr *op) {
  switch (op->type()) {
#define __(op__)           \
  case NodeTy::op__:       \
    Visit(op->As<op__>()); \
    break;

    OP_ALL_FOR_EACH(__)

    default:
      LOG(FATAL) << "unsupported type: " << op->type();
  }
}
void IRVisitor::Visit(const Stmt *op) {}

void IRVisitor::Visit(const Call *op) {
  for (auto &node : op->arguments) {
    Visit(&node);
  }
}

void IRVisitor::Visit(const Minus *op) { op->a.Accept(this); }

void IRVisitor::Visit(const IntImm *op) {}
void IRVisitor::Visit(const FloatImm *op) {}

void IRVisitor::Visit(const Tensor *op) {}
void IRVisitor::Visit(const Var *op) {}
void IRVisitor::Visit(const Constant *op) {}
void IRVisitor::Visit(const For *op) {}
void IRVisitor::Visit(const Block *op) {
  for (auto &e : op->exprs) {
    e.Accept(this);
  }
}
void IRVisitor::Visit(const IfThenElse *op) {}
void IRVisitor::Visit(const Function *op) {}
void IRVisitor::Visit(const Statement *op) { op->expr.Accept(this); }
void IRVisitor::Visit(const Allocate *op) {}
void IRVisitor::Visit(const Param *op) {}
void IRVisitor::Visit(const Tanh *op) {
  CHECK(op->a.valid());
  Visit(&op->a);
}
void IRVisitor::Visit(const Sigmoid *op) {
  CHECK(op->a.valid());
  Visit(&op->a);
}
void IRVisitor::Visit(const Exp *op) {
  CHECK(op->a.valid());
  Visit(&op->a);
}

}  // namespace ir
}  // namespace cinn
