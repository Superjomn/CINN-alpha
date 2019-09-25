#include "cinn/backends/code_gen_c.h"
#include <cinn/core/function.h>
#include <string>
#include "cinn/ir/ir.h"
#include "cinn/utils/string.h"

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
  os_ << ") {\n";

  Print(op->body);

  PrintIndent();
  os_ << "}";
}

void CppCodeGen::Visit(const Function *op) {
  // input arguments
  std::vector<std::string> arguments;
  for (int i = 0; i < op->inputs().size(); i++) {
    auto x = op->inputs()[i];
    CHECK(x.is_var() || x.is_tensor());
    arguments.push_back("const char* " + (x.is_var() ? x.As<ir::Var>()->name() : x.As<ir::Tensor>()->name()));
    // os_ << "char* " << op->inputs()[i].As<ir::Var>()->name() << ", ";
  }
  for (int i = 0; i < op->outputs().size() - 1; i++) {
    auto x = op->inputs()[i];
    CHECK(x.is_var() || x.is_tensor());
    arguments.push_back("const char* " + (x.is_var() ? x.As<ir::Var>()->name() : x.As<ir::Tensor>()->name()));
  }

  PrintIndent();
  os_ << StringFormat("void %s (%s) {\n", op->name().c_str(), Concat(arguments, ", ").c_str());

  indent_size_++;

  Print(op->GetTransformedExpr());

  indent_size_--;
  os_ << "}";
}

}  // namespace backends
}  // namespace cinn
