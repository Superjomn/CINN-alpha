#pragma once
/*
 * This file defines IRPrinter, it dumps the IR to human-readable texture format.
 */
#include <sstream>
#include "cinn/ir/ir_visitor.h"
#include "cinn/type.h"

namespace cinn {
namespace ir {

struct Expr;
struct Stmt;

// Dump a human-readable string from an Expr.
std::string Dump(const ir::Expr &expr);

class IRPrinter : public IRVisitor {
 protected:
  std::ostream &os_;
  const int indent_block_;
  int indent_size_{0};
  bool avoid_continuous_indent_flag_{false};

  std::string reference_braces{"[]"};

 public:
  IRPrinter(std::ostream &os, int indent_num = 2) : os_(os), indent_block_(indent_num) {}

  //! Add indent to the current line.
  void PrintIndent(bool avoid_continuous_indent = true);
  void Println() {
    os_ << "\n";
    avoid_continuous_indent_flag_ = false;
  }

  void Print(Expr);
  void Print(Block);
  void Print(Var);
  void Print(const std::string &);
  void Print(primitive_t dtype);

  void Visit(const Expr *op) override;
  void Visit(const Add *op) override;
  void Visit(const Sub *op) override;
  void Visit(const Mul *op) override;
  void Visit(const Div *op) override;
  void Visit(const Mod *op) override;
  void Visit(const Minus *op) override;
  void Visit(const Exp *op) override;
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
  void Visit(const Constant *op) override;
  void Visit(const Var *op) override;
  void Visit(const Reference *op) override;
  void Visit(const Call *op) override;
  void Visit(const Assign *op) override;
  void Visit(const Function *op) override;
  void Visit(const Allocate *op) override;

  void indent_left() { indent_size_--; }
  void indent_right() { indent_size_++; }

  void set_reference_braces(const std::string &x) { reference_braces = x; }

  virtual ~IRPrinter() = default;
};

}  // namespace ir
}  // namespace cinn
