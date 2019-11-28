#include "cinn/ir/ir_printer.h"
#include <algorithm>
#include <string>
#include <vector>
#include "cinn/core/function.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/macros.h"
#include "cinn/utils/string.h"

namespace cinn {
namespace ir {

std::string Dump(const ir::Expr &expr) {
  std::stringstream os;
  IRPrinter printer(os);
  printer.Print(expr);
  return os.str();
}

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
void IRPrinter::Visit(const BoolImm *op) { os_ << op->val ? "true" : "false"; }

void IRPrinter::Visit(const Expr *op) { IRVisitor::Visit(op); }

void IRPrinter::Print(Expr op) { Visit(&op); }
void IRPrinter::Print(Var op) { os_ << op.name(); }
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

void IRPrinter::Visit(const Exp *op) {
  os_ << "exp(";
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

void IRPrinter::Visit(const Tensor *op) {
  std::vector<std::string> dims;
  for (auto &constant : op->dims()) {
    if (constant.value_set()) {
      dims.push_back(std::to_string(constant.int_val()));
    } else {
      dims.push_back(constant.name());
    }
  }
  CHECK(!dims.empty()) << "the Tensor don't has shape";
  os_ << op->name() << StringFormat("<%s>", Concat(dims, ",").c_str());
}

void IRPrinter::Visit(const For *op) {
  //@{
  os_ << "for(";
  Print(op->iterator);
  os_ << ", ";
  Print(op->iter_init);
  Print(", ");
  Print(op->iter_cond);
  Print(", ");
  Print(op->iter_inc);
  os_ << ") {";
  Println();
  //@}
  indent_right();

  // print a block
  CHECK(op->body.is_block());
  Print(op->body);
  Println();

  indent_left();

  //@{
  PrintIndent();
  os_ << "}";
  //@}
}

void IRPrinter::Visit(const IfThenElse *op) {
  PrintIndent();
  Print("if(");
  Print(op->condition);
  Print(") {");
  Println();

  indent_right();
  PrintIndent();
  Print(op->true_block);
  indent_left();

  Println();

  PrintIndent();
  os_ << "}";
  Println();

  if (op->false_block.valid()) {
    Print("else");
    Println();

    PrintIndent();
    os_ << "{";
    Println();
    indent_right();

    Print(op->false_block);

    indent_left();
    os_ << "}";
    Println();
  }
}

void IRPrinter::Visit(const Block *op) {
  // PrintIndent();
  // os_ << "{%_B" << indent_size_ << "\n";

  for (size_t i = 0; i < op->body.size(); i++) {
    auto &expr = op->body[i];
    PrintIndent();
    Print(expr);
    if (i != op->body.size() - 1) Println();
  }

  // PrintIndent();
  // os_ << "}%_B" << indent_size_;
}
void IRPrinter::Visit(const Constant *op) {
  if (!op->value_set()) {
    os_ << op->name();
    return;
  }
  switch (op->ptype()) {
    case primitive_t::int32:
      os_ << op->int32_val();
      break;
    case primitive_t::int64:
      os_ << op->int64_val();
      break;
    default:
      LOG(FATAL) << "unsupported type " << op->ptype();
  }
}
void IRPrinter::Visit(const Var *op) { Print(*op); }
void IRPrinter::Visit(const Reference *op) {
  CHECK_EQ(reference_braces.size(), 2UL);
  Print(op->target);
  os_ << reference_braces[0];
  for (int i = 0; i < op->iterators.size() - 1; i++) {
    Print(op->iterators[i]);
    os_ << ",";
  }
  if (op->iterators.size() >= 1) {
    Print(op->iterators.back());
  }
  os_ << reference_braces[1];
}

void IRPrinter::Visit(const Call *op) {
  os_ << op->caller;
  os_ << "(";
  for (int i = 0; i < op->arguments.size() - 1; i++) {
    Print(op->arguments[i]);
    os_ << ", ";
  }
  if (op->arguments.size() > 1) {
    Print(op->arguments.back());
  }
  os_ << ");";
}

void IRPrinter::Visit(const Assign *op) {
  // PrintIndent();
  Print(op->a);
  os_ << " = ";
  Print(op->b);
  os_ << ";";
}

void IRPrinter::Visit(const ir::Function *op) {
  LOG_INDENT(6);
  CINN_DEBUG(3) << "print function " << op->name();

  // print func definition.
  std::vector<std::string> arguments;
  for (int i = 0; i < op->inputs.size(); i++) {
    auto &x = op->inputs[i];
    CHECK(x.is_var() || x.is_tensor());
    if (x.is_var())
      arguments.push_back("Buffer& " + x.As<ir::Var>()->name());
    else
      arguments.push_back("Tensor& " + x.As<ir::Tensor>()->name());
  }
  for (int i = 0; i < op->outputs.size(); i++) {
    auto &x = op->outputs[i];
    CHECK(x.is_var() || x.is_tensor());
    if (x.is_var())
      arguments.push_back("Buffer& " + x.As<ir::Var>()->name());
    else
      arguments.push_back("Tensor& " + x.As<ir::Tensor>()->name());
  }

  PrintIndent();
  os_ << StringFormat("def %s (%s)", op->name().c_str(), arguments.empty() ? "" : Concat(arguments, ", ").c_str());

  // body print with indent
  PrintIndent();
  os_ << " {";
  Println();

  indent_right();

  // print the buffer allocate.

  PrintIndent();
  Print(op->body);
  os_ << '\n';

  indent_left();

  PrintIndent();
  os_ << "}";
}

void IRPrinter::Visit(const Allocate *op) {
  PrintIndent();
  os_ << "Buffer " << op->buffer_name << "(";
  Print(op->size);
  os_ << ", ";
  Print(op->dtype);
  os_ << ");";
}

void IRPrinter::Print(primitive_t dtype) {
  switch (dtype) {
#define TYPE_CASE(type__)   \
  case primitive_t::type__: \
    os_ << #type__ "_t";    \
    break;

    PRIMITIVE_TYPE_FOR_EACH(TYPE_CASE)
#undef TYPE_CASE
  }
}

void IRPrinter::PrintIndent(bool avoid_continuous_indent) {
  if (avoid_continuous_indent && !avoid_continuous_indent_flag_) {
    os_ << std::string(indent_block_ * indent_size_, ' ');
    avoid_continuous_indent_flag_ = true;
  }
}

void IRPrinter::Visit(const Let *op) {
  CHECK(!op->is_unk());
  // print type
  os_ << op->ctype() << " ";
  os_ << op->ptype();

  // print var
  CHECK(op->a.is_var());
  if (op->a.As<ir::Var>()->is_reference()) {
    os_ << "&";
  }
  os_ << " ";
  Print(op->a);

  os_ << " = ";

  Print(op->b);
  os_ << ";";
}
void IRPrinter::Visit(const SumAssign *op) {
  Print(op->a);
  os_ << " += ";
  Print(op->b);
  os_ << ";";
}
void IRPrinter::Visit(const SubAssign *op) {
  Print(op->a);
  os_ << " -= ";
  Print(op->b);
  os_ << ";";
}
void IRPrinter::Visit(const MulAssign *op) {
  Print(op->a);
  os_ << " *= ";
  Print(op->b);
  os_ << ";";
}
void IRPrinter::Visit(const DivAssign *op) {
  Print(op->a);
  os_ << " /= ";
  Print(op->b);
  os_ << ";";
}

void IRPrinter::Visit(const BufferOpr *op) {
  switch (op->operation) {
    case BufferOpr::Opr::kCreate:
      os_ << StringFormat("%s = create_buffer()", op->name.c_str());
      break;
    case BufferOpr::Opr::kDestroy:
      os_ << StringFormat("destroy_buffer(%s)", op->name.c_str());
      break;
    case BufferOpr::Opr::kReference:
      os_ << StringFormat("%s", op->name.c_str());
      break;
    case BufferOpr::Opr::kCreateAssign:
      os_ << StringFormat("create_assign_buffer(%s)", op->name.c_str());
      break;
  }
}

void IRPrinter::Visit(const Mark *op) { os_ << "// " << op->content; }
void IRPrinter::Visit(const Identity *op) {
  os_ << "#" << op->id << "(";
  Print(op->expr);
  os_ << ")";
}

void IRPrinter::Visit(const Array *op) {
  os_ << op->name << "<";
  Print(op->size);
  os_ << ">";
}

void IRPrinter::Visit(const SIMDOpr *op) {
  switch (op->opr) {
    case ir::SIMDOpr::Opr::kAdd:
      os_ << "simd_add_" << op->vector_width << "(";
      break;
    case ir::SIMDOpr::Opr::kSub:
      os_ << "simd_sub_" << op->vector_width << "(";
      break;
    case ir::SIMDOpr::Opr::kMul:
      os_ << "simd_mul_" << op->vector_width << "(";
      break;
    case ir::SIMDOpr::Opr::kDiv:
      os_ << "simd_div_" << op->vector_width << "(";
      break;
    case ir::SIMDOpr::Opr::kLoad:
      os_ << "simd_load" << op->vector_width << "(";
      break;
    case ir::SIMDOpr::Opr::kStore:
      os_ << "simd_store" << op->vector_width << "(";
      break;
  }

  if (op->opr == ir::SIMDOpr::Opr::kLoad) {
    Print(op->a);
    os_ << ")";
  } else {
    Print(op->a);
    os_ << ", ";
    Print(op->b);
    os_ << ")";
  }
}

void IRPrinter::Visit(const Cast *op) {
  os_ << "cast<" << op->ptype() << ", " << op->ctype() << ">";
  os_ << "(";
  Print(op->expr);
  os_ << ")";
}

void IRPrinter::Visit(const Module *op) {
  if (op->global_data_section.valid()) {
    Print(op->global_data_section);
    os_ << "\n\n";
  }
  if (op->function_section.valid()) Print(op->function_section);
}

void IRPrinter::Visit(const CallOnce *op) {
  PrintIndent();
  os_ << "call_once { //" << op->cond_var_name;
  Println();

  indent_right();
  Print(op->block);
  indent_left();

  Println();
  PrintIndent();
  os_ << "}";
}

}  // namespace ir
}  // namespace cinn
