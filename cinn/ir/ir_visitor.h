#pragma once

#include <iosfwd>

namespace cinn {

struct Function;

namespace ir {

#define NODE_PRE_DEFINE(x__) struct x__;

struct Var;
struct Stmt;
struct Expr;
struct Add;
struct Sub;
struct Div;
struct Mul;
struct Mod;

struct Minus;

struct NE;
struct EQ;
struct LE;
struct LT;
struct GT;
struct GE;
struct For;
struct IfThenElse;

struct And;
struct Or;

struct IntImm;
struct FloatImm;

struct Tensor;
struct Constant;
struct Reference;
struct Block;

struct Min;
struct Max;
struct Call;
struct Assign;
struct Statement;
struct Allocate;
struct Param;

struct Tanh;
struct Sigmoid;

/// Visitor pattern for IR nodes. The default one just visit their children.
class IRVisitor {
 public:
  IRVisitor() = default;

  virtual void Visit(const Expr* op);

  virtual void Visit(const Param* op);
  virtual void Visit(const Stmt* op);
  virtual void Visit(const Add* op);
  virtual void Visit(const Sub* op);
  virtual void Visit(const Mul* op);
  virtual void Visit(const Div* op);
  virtual void Visit(const Mod* op);

  virtual void Visit(const Minus* op);

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

  virtual void Visit(const Function* op);
  virtual void Visit(const Statement* op);
  virtual void Visit(const Allocate* op);

  virtual void Visit(const Tanh* op);
  virtual void Visit(const Sigmoid* op);
};

}  // namespace ir
}  // namespace cinn
