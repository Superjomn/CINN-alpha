#include "cinn/backends/code_gen_c.h"
#include <fstream>
#include <string>
#include <vector>
#include "cinn/core/function.h"
#include "cinn/core/optimize/optimizer.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/string.h"

namespace cinn {
namespace backends {

void C_CodeGen::PrintHeader() {
  os_ << "#include <stdio.h>\n";
  os_ << "\n";
  os_ << "typedef char cinn_int8_t;\n";
  os_ << "typedef long long cinn_int64_t;\n";
  os_ << "typedef unsigned char cinn_uint8_t;\n";
  os_ << "typedef unsigned int cinn_uint32_t;\n";
  os_ << "typedef unsigned long long cinn_uint64_t;\n";
  os_ << "typedef float cinn_float32_t;\n";
  os_ << "\n";
  os_ << "#define min(a,b) ((a)<(b) ? (a) : (b))\n";
  os_ << "#define max(a,b) ((a)>(b) ? (a) : (b))\n";
  Println();
  Println();
}

void C_CodeGen::PrintFileGuardHeader() {
  os_ << "#ifndef " << file_guard;
  Println();
  os_ << "#define " << file_guard;
  Println();
}

void C_CodeGen::PrintFileGuardFooter() { os_ << "\n\n#endif  // " << file_guard << "\n"; }

void C_CodeGen::Visit(const ir::For *op) {
  os_ << "for (int ";
  Print(op->iterator);
  os_ << " = ";
  Print(op->iter_init);
  Print("; ");

  Print(op->iter_cond);
  Print("; ");

  Print(op->iterator);
  os_ << " += ";
  Print(op->iter_inc);
  os_ << ") {";
  Println();

  indent_right();
  //@{ a block
  Print(op->body);
  Println();
  //@}
  indent_left();

  PrintIndent();
  os_ << "}";
}

void C_CodeGen::Visit(const Function *op) {
  // input arguments
  std::vector<std::string> arguments;

  auto collect_argument = [&](Expr &x) {
    CHECK(x.is_var() || x.is_tensor());
    auto name = x.is_var() ? x.As<ir::Var>()->name() : x.As<ir::Tensor>()->name();
    arguments.push_back(StringFormat("cinn_%s_t* %s", ptype_to_str(x.ptype()).c_str(), name.c_str()));
  };

  for (int i = 0; i < op->inputs().size(); i++) {
    auto x = op->inputs()[i];
    collect_argument(x);
  }
  for (int i = 0; i < op->outputs().size(); i++) {
    auto x = op->outputs()[i];
    collect_argument(x);
  }

  CHECK(!arguments.empty()) << "no function argument is provided";

  PrintIndent();
  auto definition = StringFormat("void %s (%s)", op->name().c_str(), Concat(arguments, ", ").c_str());

  if (compile_mode_ == Mode::source) {
    os_ << definition << " {";
    Println();

    auto expr = op->ComputeTransformedExpr();

    indent_right();
    //@{ a block
    PrintIndent();
    Print(expr);
    //@}
    indent_left();

    Println();
    PrintIndent();
    os_ << "}";

  } else if (compile_mode_ == Mode::header) {
    os_ << definition << ";";
    Println();
  } else {
    LOG(FATAL) << "not supported compile mode";
  }
}

void C_CodeGen::Visit(const ir::Reference *op) {
  Print(op->target);
  os_ << "[";
  std::vector<std::string> iterators;
  for (auto &iter : op->iterators) {
    iterators.push_back(ir::Dump(iter));
  }
  os_ << Concat(iterators, ", ");
  os_ << "]";
}

void C_CodeGen::Visit(const ir::Tensor *op) { os_ << op->name(); }

void C_CodeGen::operator()(const ir::Expr &expr) {
  optimizer(const_cast<Expr *>(&expr));

  PrintFileGuardHeader();
  PrintHeader();
  Print(expr);
  PrintFileGuardFooter();
}

void C_CodeGen::WriteToFile(const std::string &path) const {
  CINN_DEBUG(0) << "Write " << path;
  std::ofstream file(path);
  CHECK(file.is_open()) << "failed to open file " << path;
  file << compiled_code();
  file.close();
}

void C_CodeGen::Visit(const ir::Block *op) {
  for (size_t i = 0; i < op->exprs.size(); i++) {
    auto &expr = op->exprs[i];
    PrintIndent();
    Print(expr);

    if (i != op->exprs.size() - 1) Println();
  }
}

void CompileAsC(const ir::Expr &expr, const std::string &header_file, const std::string &source_file) {
  CHECK(!header_file.empty()) << "header file path is empty";
  CHECK(!source_file.empty()) << "source file path is empty";
  {  // write source file
    C_CodeGen gen(/*is source*/ true);
    gen(expr);
    gen.WriteToFile(source_file);
  }

  {  // write header file
    C_CodeGen gen(/*is source*/ false);
    gen(expr);

    gen.WriteToFile(header_file);
  }
}

}  // namespace backends
}  // namespace cinn
