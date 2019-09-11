#include "cinn/backends/cpp_code_gen.h"
#include <cinn/core/function.h>
#include <string>
#include "cinn/ir/ir.h"

namespace cinn {
namespace backends {

void CppCodeGen::Visit(const ir::For *op) {
  os_ << "for(int ";
  Print(op->iterator);
  os_ << " = ";
  Print(op->iter_init);
  Print("; ");

  Print(op->iter_cond);
  Print("; ");

  Print(op->iterator);
  os_ << " += ";
  Print(op->iter_inc);
  os_ << ")";
  Print(op->body);
}

void CppCodeGen::Visit(const Function *op) {
  os_ << "void " << op->name << "(";
  // input arguments
  int i;
  for (i = 0; i < op->inputs.size(); i++) {
    CHECK(op->inputs[i].is_var());
    os_ << "char* " << op->inputs[i].As<ir::Var>()->name() << ", ";
  }
  // output arguments
  for (i = 0; i < op->outputs.size() - 1; i++) {
    CHECK(op->outputs[i].is_var());
    os_ << "char * " << op->outputs[i].As<ir::Var>()->name() << ", ";
  }
  if (op->outputs.size() >= 1) {
    CHECK(op->outputs[i].is_var());
    os_ << "char * " << op->outputs[i].As<ir::Var>()->name();
  }
  os_ << ") ";

  // body print with indent
  int current_indent = indent_size_;
  indent_size_++;
  os_ << "" << std::string(indent_block_ * current_indent, ' ') << "{\n";
  os_ << std::string(indent_block_ * (current_indent + 1), ' ');
  Print(op->GetTransformedExpr());
  os_ << "\n";
  os_ << std::string(indent_block_ * current_indent, ' ') << "}";
  indent_size_--;
}

}  // namespace backends
}  // namespace cinn
