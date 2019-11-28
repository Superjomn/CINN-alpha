#pragma once
#include <map>
#include <string>
#include <tuple>
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

//! Determines whether a block can be vectorized.
bool Vectorizable(const Expr &expr, const std::set<int> &vectorize_widths, int *vector_width);

//! NOTE Only support basic expressions.
bool BasicExprContainsOnlySIMDReleatedOpr(const Expr &expr);

/**
 * Tell whether all the variables in a basic expression can pass to SIMD.
 * if the innermost forloop's iterator is i, the Rules:
 *
 * 1. A[x][i]: true
 * 2. A[i][x]: false
 * 3. A[x]: true, as a scalar
 *
 *
 * all in all, a basic expression is SIMD supported if the references contains the iterator as the last indice or not
 * contain the iterator.
 */
bool BasicExprVarsCanPassToSIMD(const Expr &basic_expr, const Expr &iterator);

//! Cast the argument of an assign expression, whose left argument is a SIMD opr, the right argument is a scalar.
//! e.g. __m128 a = 1 will be casted to __m128 a = cast<simd>(1).
void CastAssignLeftSimdArgument(ir::Expr *expr);

//! Cast the argument of an assign expression, whose right argument is a SIMD opr, while the left argument is a scalar.
//! e.g. "float a = __m128_val" will be casted to "float a = simd_reduce(__m128_val).
void CastAssignRightSimdArgument(ir::Expr *expr);

//! cast arguments for SIMD oprations + - * /
void CastSimdBasicExprOprArgument(ir::Expr *expr);

//! is the reference can be casted to a SIMD from its address.
bool IsReferenceExprSIMDLoadable(const Expr &expr, ir::Expr iterator);

bool IsSimdData(ir::Expr expr);

/**
 * Vectorize this for expression.
 * It will perform following operations:
 * 1. Replace all the opr with SIMD opr
 * 2. Collect all the references, and transpose them to SIMD cast arguments
 * 3. Remove the outer foorloop.
 */
class Vectorize {
 public:
  void operator()(int vector_width, ir::Expr *for_expr);

 private:
  /**
   * Vectorize the operation expressions.
   * @param block_expr the block expression.
   * @param vector_width the size of vectorize.
   *
   * Such as
   *    Add(expression) to SIMDOpr::Add
   */

  static void VectorizeOperations(ir::Expr *block_expr, int vector_width);

  static void AddNecessaryCastForSimdData(ir::Expr *block_expr, ir::Expr iterator, int vector_width);

  /**
   * Collect the references used as SIMD arguments.
   * @param block_expr the block of the forloop.
   * @return the collected reference expression those used in SIMDOpr.
   */
  static std::map<std::string, ir::Expr> CollectSimdReferences(const ir::Expr &block_expr);

  /**
   * Replace the vectorized forloop's iterator to zero(for the forloop will be removed latter).
   *
   * @param for_expr the forloop expression to modify.
   */
  static void ReplaceForIterToZero(ir::Expr *for_expr);

  /**
   * Insert the SIMD arguments cast expressions to the block.
   * @param block_expr the block to modify.
   * @param vector_width the size of vectorize.
   * @param simd_ref_args the collected reference expressions used in SIMDOpr.
   * @return a map from the string representation of reference expressions to the corresponding SIMD data variable
   * expressions.
   */
  static std::map<std::string, ir::Expr> InsertSimdCastExprToBlock(ir::Expr *block_expr,
                                                                   int vector_width,
                                                                   const std::map<std::string, Expr> &simd_ref_args);

  //! Replace the references to SIMD arguments.
  static void ReplaceSimdArgument(const std::map<std::string, Expr> &ref_to_vars, ir::Expr *block_expr);

  static void RemoveForloop(ir::Expr *for_expr);

 private:
  int vector_width_;
};

}  // namespace optimize
}  // namespace cinn
