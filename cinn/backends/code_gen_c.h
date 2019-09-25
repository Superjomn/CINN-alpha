#pragma once
#include "cinn/core/optimize/optimizer.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"

namespace cinn {
namespace backends {

class CppCodeGen : public ir::IRPrinter {
  const char* file_guard = "CINN_FILE_";
  Optimizer optimizer;

 public:
  explicit CppCodeGen(std::ostream& os) : ir::IRPrinter(os) {}

  void operator()(const ir::Expr& expr);

 private:
  //! Insert the C include header.
  void PrintHeader();

  void PrintType(primitive_t ptype);

  //! Insert file guard
  // #ifndef CINN_FILE_
  // #define CINN_FILE_
  void PrintFileGuardHeader();
  //! Insert file guard footer.
  //! #endif  // CINN_FILE_
  void PrintFileGuardFooter();

  void Visit(const ir::For* op) override;
  void Visit(const Function* op) override;
  void Visit(const ir::Tensor* op) override;

 public:
  void Visit(const ir::Reference* op) override;
};

}  // namespace backends
}  // namespace cinn
