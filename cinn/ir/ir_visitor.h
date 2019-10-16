#pragma once

#include "cinn/core/function.h"
#include "cinn/ir/ir.h"

namespace cinn {

struct Function;

namespace ir {

/**
 * Base class of all the methods visit the IR tree.
 * @param RetTy return type.
 * @param Args type of the extra arguments passed to the all the methods.
 */
template <typename RetTy = void, typename... Args>
struct IRVisitorBase {
  virtual RetTy Visit(const ir::Expr* expr, Args... args) {
    switch (expr->type()) {
#define __(op__)         \
  case ir::NodeTy::op__: \
    return Visit(expr->As<ir::op__>(), args...);

      __(FloatImm);
      __(IntImm);

      __(Var);
      __(Param);
      __(Tensor);
      __(Reference);
      __(Statement);
      __(Allocate);

      __(Add);
      __(Sub);
      __(Mul);
      __(Div);
      __(Mod);
      __(Minus);
      __(EQ);
      __(NE);
      __(LT);
      __(LE);
      __(GT);
      __(GE);
      __(And);
      __(Or);
      __(Exp);
      __(Assign);
      __(IncreAssign);

      __(For);
      __(IfThenElse);
      __(Block);
      __(Call);

      __(Tanh);
      __(Sigmoid);
      __(Max);
      __(Min)

      case ir::NodeTy::Function:
        return Visit(expr->As<Function>(), args...);

      case ir::NodeTy::Parameter:
        return Visit(expr->As<ir::Constant>(), args...);

      default:
        LOG(FATAL) << "not supported NodeTy " << expr->type();

#undef __
    }
  }

 protected:
  virtual RetTy Visit(const ir::Var* op, Args... args) = 0;

  virtual RetTy Visit(const Param* op, Args... args) = 0;
  virtual RetTy Visit(const Stmt* op, Args... args) = 0;
  virtual RetTy Visit(const Add* op, Args... args) = 0;
  virtual RetTy Visit(const Sub* op, Args... args) = 0;
  virtual RetTy Visit(const Mul* op, Args... args) = 0;
  virtual RetTy Visit(const Div* op, Args... args) = 0;
  virtual RetTy Visit(const Mod* op, Args... args) = 0;

  virtual RetTy Visit(const Minus* op, Args... args) = 0;
  virtual RetTy Visit(const Exp* op, Args... args) = 0;

  virtual RetTy Visit(const Min* op, Args... args) = 0;
  virtual RetTy Visit(const Max* op, Args... args) = 0;

  virtual RetTy Visit(const NE* op, Args... args) = 0;
  virtual RetTy Visit(const EQ* op, Args... args) = 0;
  virtual RetTy Visit(const For* op, Args... args) = 0;
  virtual RetTy Visit(const IfThenElse* op, Args... args) = 0;
  virtual RetTy Visit(const GT* op, Args... args) = 0;
  virtual RetTy Visit(const GE* op, Args... args) = 0;
  virtual RetTy Visit(const LT* op, Args... args) = 0;
  virtual RetTy Visit(const LE* op, Args... args) = 0;
  virtual RetTy Visit(const And* op, Args... args) = 0;
  virtual RetTy Visit(const Or* op, Args... args) = 0;
  virtual RetTy Visit(const Block* op, Args... args) = 0;

  virtual RetTy Visit(const IntImm* op, Args... args) = 0;
  virtual RetTy Visit(const FloatImm* op, Args... args) = 0;

  virtual RetTy Visit(const Tensor* op, Args... args) = 0;
  virtual RetTy Visit(const Constant* op, Args... args) = 0;
  virtual RetTy Visit(const Reference* op, Args... args) = 0;
  virtual RetTy Visit(const Call* op, Args... args) = 0;
  virtual RetTy Visit(const Assign* op, Args... args) = 0;
  virtual RetTy Visit(const IncreAssign* op, Args... args) = 0;

  virtual RetTy Visit(const Function* op, Args... args) = 0;
  virtual RetTy Visit(const Statement* op, Args... args) = 0;
  virtual RetTy Visit(const Allocate* op, Args... args) = 0;

  virtual RetTy Visit(const Tanh* op, Args... args) = 0;
  virtual RetTy Visit(const Sigmoid* op, Args... args) = 0;
};

/// Visitor pattern for IR nodes. The default one just visit their children.
class IRVisitor : public IRVisitorBase<void> {
 public:
  IRVisitor() = default;

  void Visit(const Expr* op) override { IRVisitorBase::Visit(op); };
  virtual void Visit(const Param* op);
  virtual void Visit(const Stmt* op);
  virtual void Visit(const Add* op);
  virtual void Visit(const Sub* op);
  virtual void Visit(const Mul* op);
  virtual void Visit(const Div* op);
  virtual void Visit(const Mod* op);

  virtual void Visit(const Minus* op);
  virtual void Visit(const Exp* op);

  virtual void Visit(const Min* op);
  virtual void Visit(const Max* op);

  virtual void Visit(const NE* op);
  virtual void Visit(const EQ* op);
  virtual void Visit(const For* op);
  virtual void Visit(const IfThenElse* op);
  virtual void Visit(const GT* op);
  virtual void Visit(const GE* op);
  virtual void Visit(const LT* op);
  virtual void Visit(const LE* op);
  virtual void Visit(const And* op);
  virtual void Visit(const Or* op);
  virtual void Visit(const Block* op);

  virtual void Visit(const IntImm* op);
  virtual void Visit(const FloatImm* op);

  virtual void Visit(const Tensor* op);
  virtual void Visit(const Constant* op);
  virtual void Visit(const Var* op);
  virtual void Visit(const Reference* op) {}
  virtual void Visit(const Call* op);
  virtual void Visit(const Assign* op);
  virtual void Visit(const IncreAssign* op);

  virtual void Visit(const Function* op);
  virtual void Visit(const Statement* op);
  virtual void Visit(const Allocate* op);

  virtual void Visit(const Tanh* op);
  virtual void Visit(const Sigmoid* op);
};

}  // namespace ir
}  // namespace cinn
