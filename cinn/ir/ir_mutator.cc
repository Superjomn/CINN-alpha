#include "cinn/ir/ir_mutator.h"
#include "cinn/core/function.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace ir {

void IRMutator::Mutate(Expr* op, Expr* expr) {
  CHECK_EQ(op, expr);

  switch (op->type()) {
#define __(op__)                  \
  case NodeTy::op__:              \
    Mutate(op->As<op__>(), expr); \
    break;

    OP_ALL_FOR_EACH(__)

    case NodeTy::Parameter:
      Mutate(op->As<Constant>(), expr);
      break;

    default:
      LOG(FATAL) << "unsupported type: " << op->type();
  }
#undef __
}

#define __(op__)                                 \
  void IRMutator::Mutate(op__* op, Expr* expr) { \
    CHECK(op->a.valid());                        \
    CHECK(op->b.valid());                        \
    Mutate(&op->a, &op->a);                      \
    Mutate(&op->b, &op->b);                      \
  }
OP_2_ARGS_FOR_EACH(__);
#undef __

#define __(op__)                                 \
  void IRMutator::Mutate(op__* op, Expr* expr) { \
    CHECK(op->a.valid());                        \
    Mutate(&op->a, &op->a);                      \
  }
OP_1_ARGS_FOR_EACH(__);
#undef __

#define __(op__) \
  void IRMutator::Mutate(op__* op, Expr* expr) {}
IMM_FOR_EACH(__);
// NODETY_CONTROL_OP_FOR_EACH(__);
__(Var);
__(Tensor);
__(Allocate);
__(Param);
__(Constant);
#undef __

void IRMutator::Mutate(For* op, Expr* expr) {
  Mutate(&op->iter_init, &op->iter_init);
  Mutate(&op->iter_cond, &op->iter_cond);
  Mutate(&op->iter_inc, &op->iter_inc);
  Mutate(&op->body, &op->body);
}
void IRMutator::Mutate(IfThenElse* op, Expr* expr) {
  Mutate(&op->condition, &op->condition);
  CHECK(op->true_block.valid());
  Mutate(&op->true_block, &op->true_block);
  if (op->false_block.valid()) Mutate(&op->false_block, &op->false_block);
}
void IRMutator::Mutate(Block* op, Expr* expr) {
  CINN_DEBUG(5) << "mutating block ";
  for (auto& expr : op->exprs) {
    Mutate(&expr, &expr);
  }
}
void IRMutator::Mutate(Call* op, Expr* expr) {
  for (auto& arg : op->arguments) {
    Mutate(&arg, &arg);
  }
}
void IRMutator::Mutate(Function* op, Expr* expr) {
  CINN_DEBUG(0) << "mutating Function " << op->name();
  for (auto& arg : op->inputs()) {
    Mutate(const_cast<Expr*>(&arg), const_cast<Expr*>(&arg));
  }
  for (auto& arg : op->outputs()) {
    Mutate(const_cast<Expr*>(&arg), const_cast<Expr*>(&arg));
  }
  // TODO visit body, not support yet.
  auto* body = const_cast<Expr*>(&op->ComputeTransformedExpr());
  Mutate(body, body);
}

void IRMutator::Mutate(Reference* op, Expr* expr) {
  Mutate(&op->target, &op->target);
  for (auto& iter : op->iterators) {
    Mutate(&iter, &iter);
  }
}

}  // namespace ir
}  // namespace cinn
