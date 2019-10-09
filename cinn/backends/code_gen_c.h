#pragma once
#include "cinn/core/optimize/optimizer.h"
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
  Optimizer optimizer;
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
  void Visit(const Function* op) override;
  void Visit(const ir::Tensor* op) override;
  void Visit(const ir::Block* op) override;

 public:
  void Visit(const ir::Reference* op) override;
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
