#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace ir {

void IRPrinter::Visit(const Expr *op) { op->Accept(this); }

void IRPrinter::Print(Expr op) { op.Accept(this); }
void IRPrinter::Print(Stmt op) { op.Accept(this); }

void IRPrinter::Visit(const Add *op) {
  stream << "(";
  Print(op->a);
  stream << " + ";
  Print(op->b);
  stream << ")";
}
void IRPrinter::Visit(const Sub *op) {
  stream << "(";
  Print(op->a);
  stream << " - ";
  Print(op->b);
  stream << ")";
}
void IRPrinter::Visit(const Mul *op) {
  stream << "(";
  Print(op->a);
  stream << " * ";
  Print(op->b);
  stream << ")";
}
void IRPrinter::Visit(const Div *op) {
  stream << "(";
  Print(op->a);
  stream << " / ";
  Print(op->b);
  stream << ")";
}
void IRPrinter::Visit(const IntImm *op) { stream << op->val(); }
void IRPrinter::Visit(const FloatImm *op) { stream << op->val(); }

}  // namespace ir
}  // namespace cinn
