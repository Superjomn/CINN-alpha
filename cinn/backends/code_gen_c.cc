#include "cinn/backends/code_gen_c.h"
#include <string>
#include <vector>
#include "cinn/core/function.h"
#include "cinn/core/optimize/optimizer.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/utils/string.h"

namespace cinn {
namespace backends {

void CppCodeGen::PrintHeader() {
  os_ << "#include <stdio.h>\n";
  os_ << "\n";
  os_ << "typedef char int8_t;\n";
  os_ << "typedef long long int64_t;\n";
  os_ << "typedef unsigned char uint8_t;\n";
  os_ << "typedef unsigned int uint32_t;\n";
  os_ << "typedef unsigned long long uint64_t;\n";
  os_ << "\n\n";
}

void CppCodeGen::PrintType(primitive_t ptype) {
  switch (ptype) {
#define __(ptype, repr)    \
  case primitive_t::ptype: \
    os_ << #repr;          \
    break;

    __(int64, int64_t)
    __(int32, int)
    __(float32, float)
    __(float64, double)
    __(boolean, bool)
    __(uint8, uint8_t)
    __(uint32, uint32_t)
    __(uint64, uint64_t)

#undef __
  }
}

void CppCodeGen::PrintFileGuardHeader() {
  os_ << "#ifndef " << file_guard << "\n";
  os_ << "#define " << file_guard << "\n";
}

void CppCodeGen::PrintFileGuardFooter() { os_ << "\n\n#endif  // " << file_guard << "\n"; }

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

  auto expr = op->ComputeTransformedExpr();
  optimizer(&expr);

  Print(expr);

  indent_size_--;
  os_ << "}";
}

void CppCodeGen::Visit(const ir::Reference *op) {
  Print(op->target);
  os_ << "[";
  std::vector<std::string> iterators;
  for (auto &iter : op->iterators) {
    iterators.push_back(ir::Dump(iter));
  }
  os_ << Concat(iterators, ", ");
  os_ << "]";
}

void CppCodeGen::Visit(const ir::Tensor *op) { os_ << op->name(); }

void CppCodeGen::operator()(const ir::Expr &expr) {
  optimizer(const_cast<Expr *>(&expr));

  PrintFileGuardHeader();
  PrintHeader();
  Print(expr);
  PrintFileGuardFooter();
}

}  // namespace backends
}  // namespace cinn
