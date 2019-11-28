#include "cinn/core/optimize/unroll_utils.h"

namespace cinn {
namespace optimize {

void Unroller::operator()(ir::Expr *expr) { Convert(expr); }

bool Unroller::CanUnroll(const ir::Expr &for_expr, int specific_extent) {
  return CanUnroll(for_expr, specific_extent, specific_extent);
}

bool Unroller::CanUnroll(const ir::Expr &expr, int min_extent, int max_extent) {
  int num_elements, init_value;
  if (!ir::IsConstantFor(expr, &num_elements, &init_value)) return false;
  if (!(num_elements >= min_extent && num_elements <= max_extent)) return false;
  return true;
}

void Unroller::Convert(ir::Expr *for_expr) {
  int init_value = -1, num_element = -1;
  CHECK(ir::IsConstantFor(*for_expr, &num_element, &init_value));
  CHECK_LE(num_element, UNROLL_MAX_EXTENT);
  CHECK_GE(num_element, UNROLL_MIN_EXTENT);

  auto *for_ = for_expr->As<ir::For>();

  std::vector<ir::Expr> blocks;

  for (int i = 0; i < num_element; i++) {
    int cur_val = init_value + i;

    ir::Expr block = ir::IRDeepCopy(for_->body);
    ir::IRReplace(&block, for_->iterator, Expr(cur_val));
    // we create a if-else with condition always be true to force add a block to hold the local variables.
    // the real compiler should optimize the redundantion well.
    // ir::Expr fake_ifelse = ir::IfThenElse::make(ir::Expr(true), block);

    blocks.push_back(block);
  }

  ir::Expr new_block = ir::Block::make(std::move(blocks));
  for_expr->Reset(new_block);
}

Unroller::Unroller(int specific_extent) : specific_extent_(specific_extent) {
  CHECK_LE(specific_extent, UNROLL_MAX_EXTENT);
  CHECK_GE(specific_extent, UNROLL_MIN_EXTENT);
}

Unroller::Unroller(int min_extent, int max_extent) : min_extent_(min_extent), max_extent_(max_extent) {
  CHECK_LE(max_extent, UNROLL_MAX_EXTENT);
  CHECK_GE(min_extent, UNROLL_MIN_EXTENT);
}

}  // namespace optimize
}  // namespace cinn
