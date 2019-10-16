#include "cinn/ir/ir_mutator.h"
#include "cinn/core/function.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace ir {

void IRMutator::Visit(const ir::Expr* expr, ir::Expr* op) { IRVisitorBase::Visit(expr, op); }
void IRMutator::Visit(const Stmt* op, ir::Expr* expr) {}
void IRMutator::Visit(const Tensor* op, ir::Expr* expr) {}
void IRMutator::Visit(const Statement* op, ir::Expr* expr) {}
void IRMutator::Visit(const Allocate* op, ir::Expr* expr) {}

void IRMutator::Visit(const ir::Reference* op, ir::Expr* expr) {
  ir::Reference* m_op = expr->As<ir::Reference>();
  Visit(&m_op->target, &m_op->target);
  for (auto& iter_expr : m_op->iterators) {
    Visit(&iter_expr, &iter_expr);
  }
}

void IRMutator::Visit(const Function* op, ir::Expr* expr) {
  for (auto& arg : op->inputs()) {
    Visit(&arg, const_cast<Expr*>(&arg));
  }
  for (auto& arg : op->outputs()) {
    Visit(&arg, const_cast<Expr*>(&arg));
  }
  auto* body = const_cast<Expr*>(&op->ComputeTransformedExpr());
  Visit(body, body);
}

void IRMutator::Visit(const For* op, Expr* expr) {
  For* m_op = expr->As<For>();
  Visit(&m_op->iter_init, &m_op->iter_init);
  Visit(&m_op->iter_cond, &m_op->iter_cond);
  Visit(&m_op->iter_inc, &m_op->iter_inc);
  Visit(&m_op->body, &m_op->body);
}

void IRMutator::Visit(const IfThenElse* op, Expr* expr) {
  auto* m_op = expr->As<IfThenElse>();
  Visit(&m_op->condition, &m_op->condition);
  CHECK(op->true_block.valid());
  Visit(&m_op->true_block, &m_op->true_block);
  if (op->false_block.valid()) Visit(&m_op->false_block, &m_op->false_block);
}

void IRMutator::Visit(const Block* op, Expr* expr) {
  auto* m_op = expr->As<Block>();
  for (auto& expr : m_op->exprs) {
    Visit(&expr, &expr);
  }
}

void IRMutator::Visit(const Call* op, Expr* expr) {
  auto* m_op = expr->As<Call>();
  for (auto& arg : m_op->arguments) {
    Visit(&arg, &arg);
  }
}

}  // namespace ir
}  // namespace cinn
