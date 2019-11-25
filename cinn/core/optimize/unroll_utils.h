#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_mutator_helpers.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace optimize {

const int UNROLL_MIN_EXTENT = 2;
const int UNROLL_MAX_EXTENT = 30;
/**
 * Unroll the forloop in the whole program.
 *
 * Check the forloop is able to unroll:
 * 1. the forloop's domain is constant, that is the initial value is a constant integer, the condition is a LE and the
 * inc is constant integer too.
 * 2. the extent is no larger than the specific_extent_.
 *
 * The logic:
 *
 */
class Unroller {
  int specific_extent_{-1};
  int min_extent_{-1}, max_extent_{-1};

 public:
  Unroller() : min_extent_(UNROLL_MIN_EXTENT), max_extent_(UNROLL_MAX_EXTENT) {}
  /**
   * @param specific_extent the specific forloop extent to unroll, it will ignore the other sizes.
   */
  explicit Unroller(int specific_extent);
  Unroller(int min_extent, int max_extent);

  void operator()(ir::Expr* expr);

  /**
   * Check whether a forloop expression is able to unroll, the extent is specified.
   * @param for_expr the expression to check.
   * @param specific_extent the specific extent, the number of elements in the forloop should match.
   */
  static bool CanUnroll(const ir::Expr& for_expr, int specific_extent);
  /**
   * Check whether a forloop expression is able to unroll, the extent is limited in a range.
   * @param for_expr the expression to check.
   * @param min_extent the minimum extent.
   * @param max_extent the maximum extent>
   */
  static bool CanUnroll(const ir::Expr& for_expr, int min_extent, int max_extent);

 private:
  /**
   * Unroll a forloop expression.
   * @param for_expr
   */
  void Convert(ir::Expr* for_expr);
};

}  // namespace optimize
}  // namespace cinn
