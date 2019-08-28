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

// Dump a human-readable string from an Expr.
std::string Dump(const ir::Expr &expr);

class IRPrinter : public IRVisitor {
  std::ostream &os_;
  const int indent_block_;
  int indent_size_{0};

 public:
  IRPrinter(std::ostream &os, int indent_num = 4) : os_(os), indent_block_(indent_num) {}

  void Print(Expr);
  void Print(Block);
  void Print(Var);
  void Print(const std::string &);

  void Visit(const Expr *op) override;
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
  void Visit(const IfThenElse *op) override;
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
  void Visit(const Call *op) override;

  virtual ~IRPrinter() = default;
};

}  // namespace ir
}  // namespace cinn
