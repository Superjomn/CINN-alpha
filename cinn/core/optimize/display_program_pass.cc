#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {
class ProgramDisplayPass : public Pass<ir::Expr> {
 public:
  explicit ProgramDisplayPass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    LOG_INDENT(0);
    CINN_DEBUG(2) << "\nFinal program:\n" << *expr;
  }
};

}  // namespace cinn

REGISTER_IR_PASS(display_program, cinn::ProgramDisplayPass);
