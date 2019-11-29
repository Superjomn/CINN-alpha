#pragma once
#include "cinn/backends/x86_simd.h"
#include "cinn/core/optimize/optimizer.h"
#include "cinn/core/optimize/vectorize_utils.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"

namespace cinn {
namespace backends {

/**
 * C_CodeGen generate C source code.
 *
 * It will generate header file and source file sperately.
 */
class C_CodeGen : public ir::IRPrinter {
  enum class Mode {
    header,  // compile header file
    source,  // compile source file
  };
  const char* file_guard = "CINN_FILE_";
  IrOptimizer optimizer;
  std::stringstream ss_;
  mutable std::string compiled_code_;

  Mode compile_mode_;

 public:
  /**
   * @brief Construct a C_CodeGen object.
   * @param compile_source whether this generator generates C source code; set to false to generate header file.
   */
  explicit C_CodeGen(bool compile_source = true)
      : ir::IRPrinter(ss_), compile_mode_(compile_source ? Mode::source : Mode::header) {}

  /**
   * @brief Process an expression and generate code for it.
   * @param expr the target expression.
   */
  void operator()(const ir::Expr& expr);

  /**
   * @brief Write the generated code to a file.
   * @param path destination in the disk.
   */
  void WriteToFile(const std::string& path) const;

  //! Get the source code of the implementation of all functions.
  std::string compiled_code() const {
    if (compiled_code_.empty()) compiled_code_ = ss_.str();
    return compiled_code_;
  }

 private:
  //! Insert the C include header.
  void PrintHeader();

  //! Insert file guard
  //    #ifndef CINN_FILE_
  //    #define CINN_FILE_
  void PrintFileGuardHeader();
  //! Insert file guard footer.
  //!   #endif  // CINN_FILE_
  void PrintFileGuardFooter();

  void Visit(const ir::For* op) override;
  void Visit(const ir::Function* op) override;
  void Visit(const ir::Tensor* op) override;
  void Visit(const ir::Block* op) override;
  void Visit(const ir::Let* op) override;
  void Visit(const ir::SIMDOpr* op) override;
  void Visit(const ir::BufferOpr* op) override;
  void Visit(const ir::Cast* op) override;
  void Visit(const ir::Max* op) override;
  void Visit(const ir::Min* op) override;
  void Visit(const ir::Assign* op) override;
  void Visit(const ir::SumAssign* op) override;
  void Visit(const ir::SubAssign* op) override;
  void Visit(const ir::MulAssign* op) override;
  void Visit(const ir::DivAssign* op) override;
  void Visit(const ir::Identity* op) override;

  template <typename AssignT>
  void VisitAssignX(const AssignT* op) {
    if (optimize::IsSimdData(op->a) && optimize::IsSimdData(op->b)) {
      os_ << x86::GlobalX86SIMD(op->b.ctype()).store_ps();
      os_ << "(&";
      Print(op->a);
      os_ << ", ";
      Print(op->b);
      os_ << ");";
    } else if (!op->a.is_simd() && op->b.is_simd()) {
      Print(op->a);

      switch (op->type()) {
        case ir::NodeTy::Assign:
          os_ << " = ";
          break;
        case ir::NodeTy::SumAssign:
          os_ << " += ";
          break;
        case ir::NodeTy::SubAssign:
          os_ << " -= ";
          break;
        case ir::NodeTy::MulAssign:
          os_ << " *= ";
          break;
        case ir::NodeTy::DivAssign:
          os_ << " /= ";
          break;
        default:
          NOT_IMPLEMENT
      }

      os_ << x86::GlobalX86SIMD(op->b.ctype()).custom_reduce_add_ps();  // for default
      os_ << "(";
      Print(op->b);
      os_ << ");";
      LOG(WARNING) << "to refine here";
    } else {
      IRPrinter::Visit(op);
    }
  }

 private:
  /**
   * Print the primitive type in code.
   * @param ptype
   */
  void PrintPType(primitive_t ptype);

  void Visit(const ir::Reference* op) override;

  static const char* simd_128_type;
  static const std::vector<std::string> simd_128_intrics;

 private:
  //! We record the last block to help preappend some let expression of temporary variables.
  ir::Block* last_block_{};
};

/**
 * @brief Generate C source file and write the header file and source file to disk.
 * @param expr: expression.
 * @param header_file: the path to the header file destination.
 * @param source_file: the path to the source file destination.
 */
void CompileAsC(const ir::Expr& expr, const std::string& header_file, const std::string& source_file);

}  // namespace backends
}  // namespace cinn
