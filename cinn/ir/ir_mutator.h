/**
 * IRMutator helps to traverse the expression tree and mutate some nodes. It is similar to the IRVisitor.
 */
#pragma once
#include "cinn/ir/ir_visitor.h"
#include "cinn/utils/macros.h"

namespace cinn {
struct Function;
namespace ir {

// Forward declare all the IR types.
#define __(op__) struct op__;
OP_ALL_WITHOUT_FUNCTION_FOR_EACH(__);
#undef __
struct Constant;

struct Expr;

class IRMutator : public IRVisitorBase<void, ir::Expr*> {
 public:
  void Visit(const ir::Expr* expr, ir::Expr* op) override;

  void Visit(const ir::IntImm* op, ir::Expr* expr) override {}
  void Visit(const ir::FloatImm* op, ir::Expr* expr) override {}
  void Visit(const ir::BoolImm* op, ir::Expr* expr) override {}

  //! A callback that visit all the basic expressions(no control flow) of the program.
  virtual Expr VisitBasicExpr(Expr* expr) { return *expr; }

// Operator with 2 parameters.
#define OP_2PARAM(op__)                                     \
  void Visit(const ir::op__* op, ir::Expr* expr) override { \
    CHECK(op->a.valid());                                   \
    CHECK(op->b.valid());                                   \
    auto* node = expr->As<op__>();                          \
    LazyUpdateExpr(&node->a, VisitBasicExpr(&node->a));     \
    LazyUpdateExpr(&node->b, VisitBasicExpr(&node->b));     \
    Visit(&node->a, &node->a);                              \
    Visit(&node->b, &node->b);                              \
  }

// Operator with 1 parameter.
#define OP_1PARAM(op__)                                     \
  void Visit(const ir::op__* op, ir::Expr* expr) override { \
    CHECK(op->a.valid());                                   \
    auto* node = expr->As<op__>();                          \
    LazyUpdateExpr(&node->a, VisitBasicExpr(&node->a));     \
    Visit(&node->a, &node->a);                              \
  }
  // OP_2PARAM(Add);
  OP_2PARAM(Sub);
  OP_2PARAM(Mul);
  OP_2PARAM(Div);
  OP_2PARAM(Mod);
  OP_1PARAM(Minus);
  OP_2PARAM(EQ);
  OP_2PARAM(NE);
  OP_2PARAM(LT);
  OP_2PARAM(LE);
  OP_2PARAM(GT);
  OP_2PARAM(GE);
  OP_2PARAM(And);
  OP_2PARAM(Or);
  OP_1PARAM(Not);

  OP_2PARAM(Min);
  OP_2PARAM(Max);

  OP_2PARAM(Assign);
  OP_2PARAM(SumAssign);
  OP_2PARAM(SubAssign);
  OP_2PARAM(MulAssign);
  OP_2PARAM(DivAssign);
  // OP_2PARAM(Let);

  void Visit(const ir::Let* op, ir::Expr* expr) override;
  void Visit(const ir::Add* op, ir::Expr* expr) override;
  OP_2PARAM(SIMDOpr);

  OP_1PARAM(Exp);
  OP_1PARAM(Tanh);
  OP_1PARAM(Sigmoid);

#undef OP_2PARAM
#undef OP_1PARAM

  void Visit(const ir::Var* op, ir::Expr* expr) override {}
  void Visit(const ir::Constant* op, ir::Expr* expr) override {}

  void Visit(const ir::Reference* op, ir::Expr* expr);

  void Visit(const Function* op, ir::Expr* expr) override;

  void Visit(const For* op, Expr* expr) override;
  void Visit(const IfThenElse* op, Expr* expr) override;
  void Visit(const Block* op, Expr* expr) override;
  void Visit(const Call* op, Expr* expr) override;

  void Visit(const Stmt* op, Expr* expr) override;
  void Visit(const Tensor* op, Expr* expr) override;
  void Visit(const Statement* op, Expr* expr) override;
  void Visit(const Allocate* op, Expr* expr) override;

  void Visit(const Mark* p, Expr* expr) override {}
  void Visit(const BufferOpr* op, Expr* expr) override;
  void Visit(const Cast* op, Expr* expr) override;
  void Visit(const Array* op, Expr* expr) override;
  void Visit(const Module* op, Expr* expr) override;
  void Visit(const CallOnce* op, Expr* expr) override;

 protected:
  //! Update the expression if their data address is different.
  bool LazyUpdateExpr(Expr* expr, const Expr& e);
};

}  // namespace ir
}  // namespace cinn
