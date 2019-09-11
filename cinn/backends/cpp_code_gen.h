#pragma once
#include "cinn/ir/ir_printer.h"

namespace cinn {
namespace backends {

class CppCodeGen : public ir::IRPrinter {
 public:
  explicit CppCodeGen(std::ostream& os) : ir::IRPrinter(os) {}

  void Visit(const ir::For* op) override;

  void Visit(const Function* op) override;
};

}  // namespace backends
}  // namespace cinn
