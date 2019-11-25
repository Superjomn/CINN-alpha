#include "cinn/backends/code_gen_c.h"
#include <fstream>
#include <string>
#include <vector>
#include "cinn/backends/x86_simd.h"
#include "cinn/core/function.h"
#include "cinn/core/optimize/optimizer.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/string.h"

namespace cinn {
namespace backends {

void C_CodeGen::PrintHeader() {
  os_ << "#include <immintrin.h>\n";
  os_ << "#include <math.h>\n";
  os_ << "#include <stdio.h>\n";
  os_ << "\n";
  os_ << "typedef bool cinn_boolean_t;\n";
  os_ << "typedef char cinn_int8_t;\n";
  os_ << "typedef int cinn_int32_t;\n";
  os_ << "typedef long long cinn_int64_t;\n";
  os_ << "typedef unsigned char cinn_uint8_t;\n";
  os_ << "typedef unsigned int cinn_uint32_t;\n";
  os_ << "typedef unsigned long long cinn_uint64_t;\n";
  os_ << "typedef float cinn_float32_t;\n";
  os_ << "\n";
  os_ << "#define cinn_min(a,b) ((a)<(b) ? (a) : (b))\n";
  os_ << "#define cinn_max(a,b) ((a)>(b) ? (a) : (b))\n";
  os_ << "#define cinn_copy(a,b,size) memcpy((b), (a), (size))\n";
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

void C_CodeGen::Visit(const ir::Function *op) {
  // input arguments
  std::vector<std::string> arguments;

  auto collect_argument = [&](Expr &x) {
    CHECK(x.is_var() || x.is_tensor());
    auto name = x.is_var() ? x.As<ir::Var>()->name() : x.As<ir::Tensor>()->name();
    arguments.push_back(StringFormat("cinn_%s_t* %s", ptype_to_str(x.ptype()).c_str(), name.c_str()));
  };

  for (int i = 0; i < op->inputs.size(); i++) {
    auto x = op->inputs[i];
    collect_argument(x);
  }
  for (int i = 0; i < op->outputs.size(); i++) {
    auto x = op->outputs[i];
    collect_argument(x);
  }

  PrintIndent();
  auto definition =
      StringFormat("void %s (%s)", op->name().c_str(), arguments.empty() ? "" : Concat(arguments, ", ").c_str());

  if (compile_mode_ == Mode::source) {
    os_ << definition << " {";
    Println();

    indent_right();
    //@{ a block
    PrintIndent();
    Print(op->body);
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
  if (compile_mode_ == Mode::source) {
    optimizer(const_cast<Expr *>(&expr));
  }

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
  // PrintIndent();
  // os_ << "{\n";
  for (size_t i = 0; i < op->body.size(); i++) {
    auto &expr = op->body[i];
    PrintIndent();
    Print(expr);

    if (i != op->body.size() - 1) Println();
  }
  // PrintIndent();
  // os_ << "}\n";
}

void CompileAsC(const ir::Expr &expr, const std::string &header_file, const std::string &source_file) {
  CHECK(!header_file.empty()) << "header file path is empty";
  CHECK(!source_file.empty()) << "source file path is empty";
  {  // write source file
    C_CodeGen gen(/*is source*/ true);
    gen(expr);
    LOG(INFO) << "******** after gen expr:\n" << expr;
    gen.WriteToFile(source_file);
  }

  {  // write header file
    C_CodeGen gen(/*is source*/ false);
    gen(expr);

    gen.WriteToFile(header_file);
  }
}

void C_CodeGen::Visit(const ir::Let *op) {
  auto simd = [&](composite_t ctype) {
    switch (ctype) {
      case composite_t::simd128:
        os_ << GlobalContext().x86_simd_128.packed_float_t();
        break;
      case composite_t::simd256:
        os_ << GlobalContext().x86_simd_256.packed_float_t();
        break;
      default:
        NOT_IMPLEMENT
    }

    if (op->a.As<ir::Var>()->is_reference()) os_ << "&";

    os_ << " ";
    Print(op->a);
    os_ << " = ";
    Print(op->b);
    os_ << ";";
  };

  switch (op->ctype()) {
    case composite_t::primitive:
      PrintPType(op->ptype());
      os_ << " ";
      Print(op->a);
      os_ << " = ";
      Print(op->b);
      os_ << ";";
      break;

    case composite_t::simd128:
    case composite_t::simd256:
      simd(op->ctype());
      break;
  }
}

void C_CodeGen::PrintPType(primitive_t ptype) {
  switch (ptype) {
#define __(ptype__)               \
  case primitive_t::ptype__:      \
    os_ << "cinn_" #ptype__ "_t"; \
    break;

    __(float64);
    __(float32);
    __(int32);
    __(int64);
    __(uint8);
    __(boolean);
    default:
      LOG(FATAL) << "Unsupported type " << ptype;
  }
}

void C_CodeGen::Visit(const ir::SIMDOpr *op) {
  const X86SIMD *x86_simd{};
  if (op->vector_width == 4) {  // m128
    x86_simd = &GlobalContext().x86_simd_128;
  } else if (op->vector_width == 8) {
    x86_simd = &GlobalContext().x86_simd_256;
  }
  CHECK(x86_simd) << "not supported vector width: " << op->vector_width;

  switch (op->opr) {
    case ir::SIMDOpr::Opr::kAdd:
      os_ << x86_simd->add_ps();
      break;
    case ir::SIMDOpr::Opr::kSub:
      os_ << x86_simd->sub_ps();
      break;
    case ir::SIMDOpr::Opr::kMul:
      os_ << x86_simd->mul_ps();
      break;
    case ir::SIMDOpr::Opr::kDiv:
      os_ << x86_simd->div_ps();
      break;
  }

  os_ << "(";

  Print(op->a);
  os_ << ", ";
  Print(op->b);
  os_ << ")";
}

void C_CodeGen::Visit(const ir::Cast *op) {
  switch (op->ctype()) {
    case composite_t::primitive:
      os_ << "(";
      PrintPType(op->ptype());
      os_ << ")";
      os_ << "(";
      Print(op->expr);
      os_ << ")";
      break;
    case composite_t::simd128:
      os_ << "*(" << GlobalContext().x86_simd_128.packed_float_t() << "*)(&";
      Print(op->expr);
      os_ << ")";
      break;
    case composite_t::simd256:
      os_ << "*(" << GlobalContext().x86_simd_256.packed_float_t() << "*)(&";
      Print(op->expr);
      os_ << ")";
      break;
  }
}

std::string GetDataRepr(const ir::BufferOpr *op) {
  switch (op->ptype()) {
    case primitive_t::float32:
      return Concat(ToString(op->assigned_data.get<std::vector<float32_t>>()), ",");
    case primitive_t::int32:
      return Concat(ToString(op->assigned_data.get<std::vector<int32_t>>()), ",");
    case primitive_t::int8:
      return Concat(ToString(op->assigned_data.get<std::vector<int8_t>>()), ",");
    case primitive_t::uint8:
      return Concat(ToString(op->assigned_data.get<std::vector<uint8_t>>()), ",");
    default:
      LOG(FATAL) << "Not supported ptype: " << op->ptype();
  }
}

void C_CodeGen::Visit(const ir::BufferOpr *op) {
  switch (op->operation) {
    case ir::BufferOpr::Opr::kCreate:
      PrintPType(op->ptype());
      os_ << "* ";
      os_ << op->name;
      os_ << " = ";
      os_ << " (";
      PrintPType(op->ptype());
      os_ << "*) malloc(";
      Print(op->size);
      os_ << ");";
      break;

    case ir::BufferOpr::Opr::kCreateAssign:
      PrintPType(op->ptype());
      os_ << " ";
      os_ << op->name;
      os_ << "[] = {";
      os_ << GetDataRepr(op);
      os_ << "};";
      break;

    case ir::BufferOpr::Opr::kDestroy:
      os_ << "free " << op->name << ";";
      break;

    case ir::BufferOpr::Opr::kReference:
      os_ << op->name;
      break;
  }
}

void C_CodeGen::Visit(const ir::Max *op) {
  os_ << "cinn_max(";
  Print(op->a);
  os_ << ", ";
  Print(op->b);
  os_ << ")";
}
void C_CodeGen::Visit(const ir::Min *op) {
  os_ << "cinn_min(";
  Print(op->a);
  os_ << ", ";
  Print(op->b);
  os_ << ")";
}
}  // namespace backends
}  // namespace cinn
