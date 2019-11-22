#include "cinn/ir/ir_mutator.h"
#include "cinn/core/function.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace ir {

void IRMutator::Visit(const ir::Expr* expr, ir::Expr* op) { IRVisitorBase::Visit(expr, op); }
void IRMutator::Visit(const Stmt* op, ir::Expr* expr) { NOT_IMPLEMENT }
void IRMutator::Visit(const Tensor* op, ir::Expr* expr) {}
void IRMutator::Visit(const Statement* op, ir::Expr* expr) { NOT_IMPLEMENT }
void IRMutator::Visit(const Allocate* op, ir::Expr* expr) {
  auto* node = expr->As<Allocate>();
  LazyUpdateExpr(&node->size, VisitBasicExpr(&node->size));
  Visit(&node->size, &node->size);
}

void IRMutator::Visit(const ir::Reference* op, ir::Expr* expr) {
  ir::Reference* m_op = expr->As<ir::Reference>();
  Visit(&m_op->target, &m_op->target);
  for (auto& iter_expr : m_op->iterators) {
    LazyUpdateExpr(&iter_expr, VisitBasicExpr(&iter_expr));
    Visit(&iter_expr, &iter_expr);
  }
}

void IRMutator::Visit(const Function* op, ir::Expr* expr) {
  auto* node = expr->As<Function>();
  for (auto& arg : node->inputs) {
    LazyUpdateExpr(&arg, VisitBasicExpr(&arg));
    Visit(&arg, const_cast<Expr*>(&arg));
  }
  for (auto& arg : node->outputs) {
    LazyUpdateExpr(&arg, VisitBasicExpr(&arg));
    Visit(&arg, &arg);
  }
  Visit(&node->body, &node->body);
}

void IRMutator::Visit(const For* op, Expr* expr) {
  For* m_op = expr->As<For>();
  LazyUpdateExpr(&m_op->iter_init, VisitBasicExpr(&m_op->iter_init));
  Visit(&m_op->iter_init, &m_op->iter_init);

  LazyUpdateExpr(&m_op->iter_cond, VisitBasicExpr(&m_op->iter_cond));
  Visit(&m_op->iter_cond, &m_op->iter_cond);

  LazyUpdateExpr(&m_op->iter_inc, VisitBasicExpr(&m_op->iter_inc));
  Visit(&m_op->iter_inc, &m_op->iter_inc);

  LazyUpdateExpr(&m_op->body, VisitBasicExpr(&m_op->body));
  Visit(&m_op->body, &m_op->body);
}

void IRMutator::Visit(const IfThenElse* op, Expr* expr) {
  auto* m_op = expr->As<IfThenElse>();

  LazyUpdateExpr(&m_op->condition, VisitBasicExpr(&m_op->condition));
  Visit(&m_op->condition, &m_op->condition);

  CHECK(op->true_block.valid());
  Visit(&m_op->true_block, &m_op->true_block);
  if (op->false_block.valid()) Visit(&m_op->false_block, &m_op->false_block);
}

void IRMutator::Visit(const Block* op, Expr* expr) {
  auto* m_op = expr->As<Block>();
  for (auto& expr : m_op->body) {
    Visit(&expr, &expr);
  }
}

void IRMutator::Visit(const Call* op, Expr* expr) {
  auto* m_op = expr->As<Call>();
  for (auto& arg : m_op->arguments) {
    LazyUpdateExpr(&arg, VisitBasicExpr(&arg));
    Visit(&arg, &arg);
  }
}

void IRMutator::Visit(const BufferOpr* op, Expr* expr) {
  auto* m_op = expr->As<BufferOpr>();
  LazyUpdateExpr(&m_op->size, VisitBasicExpr(&m_op->size));
  Visit(&m_op->size, &m_op->size);
}

void IRMutator::Visit(const Array* op, Expr* expr) {
  auto* m_op = expr->As<Array>();
  LazyUpdateExpr(&m_op->size, VisitBasicExpr(&m_op->size));
  Visit(&m_op->size, &m_op->size);
}

void IRMutator::Visit(const Cast* op, Expr* expr) {
  auto* m_op = expr->As<Cast>();
  Visit(&m_op->expr, &m_op->expr);
}

bool IRMutator::LazyUpdateExpr(Expr* expr, const Expr& e) {
  if (expr->ptr() != e.ptr()) {
    expr->set_ptr(e.ptr());
  }
}

void IRMutator::Visit(const ir::Let* op, ir::Expr* expr) {
  // LOG(INFO) << "mutate let: " << ir::Dump(*expr);
  CHECK(op->a.valid());
  CHECK(op->b.valid());
  auto* node = expr->As<ir::Let>();
  // LazyUpdateExpr(&node->a, VisitBasicExpr(&node->a));
  // LazyUpdateExpr(&node->b, VisitBasicExpr(&node->b));
  Visit(&node->a, &node->a);
  Visit(&node->b, &node->b);
}

void IRMutator::Visit(const ir::Add* op, ir::Expr* expr) {
  // LOG(INFO) << "mutate let: " << ir::Dump(*expr);
  CHECK(op->a.valid());
  CHECK(op->b.valid());
  auto* node = expr->As<ir::Add>();

  LazyUpdateExpr(&node->a, VisitBasicExpr(&node->a));
  LazyUpdateExpr(&node->b, VisitBasicExpr(&node->b));
  Visit(&node->a, &node->a);
  Visit(&node->b, &node->b);
}

void IRMutator::Visit(const Module* op, Expr* expr) {
  auto* node = expr->As<ir::Module>();

  if (op->global_data_section.valid()) Visit(&node->global_data_section, &node->global_data_section);
  if (op->function_section.valid()) Visit(&node->function_section, &node->function_section);
}

void IRMutator::Visit(const CallOnce* op, Expr* expr) {
  auto* node = expr->As<ir::CallOnce>();

  Visit(&node->block, &node->block);
}

}  // namespace ir
}  // namespace cinn
