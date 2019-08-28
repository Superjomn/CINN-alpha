#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/macros.h"
#include "ir_printer.h"

namespace cinn {
namespace ir {

void IRPrinter::Visit(const Add *op) {
  os_ << "(";
  Print(op->a);
  os_ << " + ";
  Print(op->b);
  os_ << ")";
}
void IRPrinter::Visit(const Sub *op) {
  os_ << "(";
  Print(op->a);
  os_ << " - ";
  Print(op->b);
  os_ << ")";
}
void IRPrinter::Visit(const Mul *op) {
  os_ << "(";
  Print(op->a);
  os_ << " * ";
  Print(op->b);
  os_ << ")";
}
void IRPrinter::Visit(const Div *op) {
  os_ << "(";
  Print(op->a);
  os_ << " / ";
  Print(op->b);
  os_ << ")";
}
void IRPrinter::Visit(const IntImm *op) { os_ << op->val(); }
void IRPrinter::Visit(const FloatImm *op) { os_ << op->val(); }

void IRPrinter::Visit(const Expr *op) {
  switch (op->type()) {
#define FOR_NODE(op__)     \
  case NodeTy::op__:       \
    Visit(op->As<op__>()); \
    break;

    OP_1_ARGS_FOR_EACH(FOR_NODE);
    OP_2_ARGS_FOR_EACH(FOR_NODE);

    case NodeTy::Int:
      Visit(op->As<IntImm>());
      break;
    case NodeTy::Float:
      Visit(op->As<FloatImm>());
      break;
    case NodeTy::Block: {
      indent_size_++;
      Visit(op->As<Block>());
      indent_size_--;
      break;
    }
    case NodeTy::For:
      Visit(op->As<For>());
      break;
    case NodeTy::IfThenElse:
      Visit(op->As<IfThenElse>());
      break;
    default:
      LOG(FATAL) << "Unsupported NodeTy " << static_cast<int>(op->type());
  }
#undef FOR_NODE
}

void IRPrinter::Print(Expr op) { Visit(&op); }
void IRPrinter::Print(Var op) { os_ << op.name(); }
void IRPrinter::Print(Block op) { Visit(&op); }
void IRPrinter::Print(const std::string &x) { os_ << x; }

void IRPrinter::Visit(const Mod *op) {
  os_ << "(";
  Print(op->a);
  os_ << " % ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const Minus *op) {
  os_ << "(";
  os_ << "-";
  Print(op->a);
  os_ << ")";
}

void IRPrinter::Visit(const Min *op) {
  os_ << "min(";
  Print(op->a);
  os_ << ",";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const Max *op) {
  os_ << "max(";
  Print(op->a);
  os_ << ",";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const NE *op) {
  os_ << "(";
  Print(op->a);
  os_ << " != ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const EQ *op) {
  os_ << "(";
  Print(op->a);
  os_ << " == ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const GT *op) {
  os_ << "(";
  Print(op->a);
  os_ << " > ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const GE *op) {
  os_ << "(";
  Print(op->a);
  os_ << " >= ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const LT *op) {
  os_ << "(";
  Print(op->a);
  os_ << " < ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const LE *op) {
  os_ << "(";
  Print(op->a);
  os_ << " <= ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const And *op) {
  os_ << "(";
  Print(op->a);
  os_ << " && ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const Or *op) {
  os_ << "(";
  Print(op->a);
  os_ << " || ";
  Print(op->b);
  os_ << ")";
}

void IRPrinter::Visit(const Tensor *op) { os_ << "tensor<>"; }

void IRPrinter::Visit(const For *op) {
  os_ << "for(";
  Print(op->iterator);
  os_ << ", ";
  Print(op->iter_init);
  Print(", ");
  Print(op->iter_cond);
  Print(", ");
  Print(op->iter_inc);
  os_ << ")";
  Print(op->body);
}

void IRPrinter::Visit(const IfThenElse *op) {
  Print("if(");
  Print(op->condition);
  Print(")");
  Print(op->true_block);
  Print("else");
  Print(op->false_block);
}

void IRPrinter::Visit(const Block *op) {
  os_ << "{\n";
  for (auto expr : op->list) {
    os_ << std::string(indent_block_ * indent_size_, ' ');
    Print(expr);
    os_ << ";\n";
  }
  os_ << "}\n";
}
void IRPrinter::Visit(const Parameter *op) { IRVisitor::Visit(op); }
void IRPrinter::Visit(const Var *op) { IRVisitor::Visit(op); }
void IRPrinter::Visit(const Reference *op) { IRVisitor::Visit(op); }

}  // namespace ir
}  // namespace cinn
