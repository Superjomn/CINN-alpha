#pragma once
/*
 * This file defines IRPrinter, it dumps the IR to human-readable texture format.
 */
#include <sstream>
#include "cinn/ir/ir_visitor.h"

namespace cinn {
namespace ir {

struct Expr;
struct Stmt;

class IRPrinter : public IRVisitor {
  std::ostream &os_;

 public:
  IRPrinter(std::ostream &os) : os_(os) {}

  void Print(Expr);

  void Visit(const Expr *op) override;
  void Visit(const Stmt *op) override;
  void Visit(const Add *op) override;
  void Visit(const Sub *op) override;
  void Visit(const Mul *op) override;
  void Visit(const Div *op) override;
  void Visit(const Mod *op) override;
  void Visit(const Minus *op) override;
  void Visit(const Min *op) override;
  void Visit(const Max *op) override;
  void Visit(const NE *op) override;
  void Visit(const EQ *op) override;
  void Visit(const For *op) override;
  void Visit(const GT *op) override;
  void Visit(const GE *op) override;
  void Visit(const LT *op) override;
  void Visit(const LE *op) override;
  void Visit(const And *op) override;
  void Visit(const Or *op) override;
  void Visit(const Block *op) override;
  void Visit(const IntImm *op) override;
  void Visit(const FloatImm *op) override;
  void Visit(const Tensor *op) override;
  void Visit(const Parameter *op) override;
  void Visit(const Var *op) override;
  void Visit(const Reference *op) override;

  virtual ~IRPrinter() = default;
};

}  // namespace ir
}  // namespace cinn
