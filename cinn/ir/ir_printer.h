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
 protected:
  void Print(Expr op);
  void Print(Stmt op);

 public:
  explicit IRPrinter(std::ostream& os) : stream(os) {}

  void Visit(const Expr* op) override;
  void Visit(const Add* op) override;
  void Visit(const Sub* op) override;
  void Visit(const Mul* op) override;
  void Visit(const Div* op) override;

  void Visit(const IntImm* op) override;
  void Visit(const FloatImm* op) override;

 private:
  std::ostream& stream;
};

}  // namespace ir
}  // namespace cinn
