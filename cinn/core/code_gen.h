#pragma once
#include <cinn/ir/ir.h>
#include <glog/logging.h>
#include <isl/ast.h>
#include <isl/cpp.h>
#include <isl/space.h>
#include <map>
#include "cinn/core/stage.h"

namespace cinn {

// Transform ISL AST expr to CINN expression.
void IslAstExprToCinnExpr(isl_ast_expr* node, ir::Expr* expr);

// Transform ISL AST node to CINN expression.
void IslAstNodeToCinnExpr(isl_ast_node* node, cinn::ir::Expr* expr);

void GenCodeFromIslAst(isl_ast_node* node);

static isl_ast_expr* CreateIslAstIndexExpression(isl_ast_build* build, const isl::map& access) {
  CHECK(build);
  isl::map schedule = isl::manage(isl_map_from_union_map(isl_ast_build_get_schedule(build)));
  LOG(INFO) << "schedule: " << schedule;

  isl::map map = isl::manage(isl_map_reverse(schedule.copy()));
  LOG(INFO) << "schedule reversed: " << map;

  isl::pw_multi_aff iterator_map = isl::manage(isl_pw_multi_aff_from_map(map.copy()));
  LOG(INFO) << "iterator_map: " << iterator_map;

  isl::pw_multi_aff index_aff = isl::manage(isl_pw_multi_aff_from_map(access.copy()));
  LOG(INFO) << "index_aff: " << index_aff;

  isl::space model2 = iterator_map.space();
  index_aff = isl::manage(isl_pw_multi_aff_align_params(index_aff.copy(), model2.copy()));
  isl::space model = index_aff.space();
  LOG(INFO) << "model: " << model;
  iterator_map = isl::manage(isl_pw_multi_aff_align_params(iterator_map.copy(), model.copy()));
  LOG(INFO) << "iterator_map: " << iterator_map;
  iterator_map = isl::manage(isl_pw_multi_aff_pullback_pw_multi_aff(index_aff.copy(), iterator_map.copy()));
  LOG(INFO) << "iterator_map: " << iterator_map;

  isl_ast_expr* index_expr = isl_ast_build_access_from_pw_multi_aff(build, iterator_map.copy());
  return index_expr;
}

/** Replace the expr called `userid` in root to `e`.
 * e.g.
 * Replace `S0(i,j+1)` to `A[i,j+1]` where statement `S0(i,j)` is `A[i,j]`
 *
 * @root: the root of the Expr tree.
 * @userid: the name of the Expr.
 * @e: the one which replace the userid.
 */
void ReplaceUseridWithExpr(ir::Expr root, const std::string& userid, ir::Expr e);

//! Replace the Expr with the transformed indices.
//! e.g. Stage's expr is C[i,j] ...
//! e.g. with ISL transformed statement S0(c0+1, c1*2), the expr will turn to C[c0+1, c1*2]
static std::map<std::string, isl_ast_expr*> ExtractIslTransformedIndiceMap(const isl::set& iterator_domain,
                                                                           isl_ast_build* build) {
  std::map<std::string, isl_ast_expr*> iterator_map;
  isl::map identity = isl::manage(isl_set_identity(iterator_domain.copy()));
  isl::map schedule = identity;

  LOG(INFO) << "schedule: " << schedule;

  identity = identity.apply_domain(schedule);
  LOG(INFO) << "identity: " << identity;

  isl_ast_expr* idx_expr = CreateIslAstIndexExpression(build, identity);

  isl::space domain_space = iterator_domain.space();

  for (int i = 1; i < isl_ast_expr_get_op_n_arg(idx_expr); i++) {
    if (isl_space_has_dim_name(domain_space.get(), isl_dim_set, i - 1)) {
      std::string original_idx_name = isl_space_get_dim_name(domain_space.get(), isl_dim_set, i - 1);
      isl_ast_expr* transformed_index = isl_ast_expr_get_op_arg(idx_expr, i);
      iterator_map.emplace(original_idx_name, transformed_index);
      LOG(INFO) << "idx: " << original_idx_name << " " << isl_ast_expr_to_C_str(transformed_index);
    }
  }
  return iterator_map;
}

static isl_ast_node* node_info_collector(isl_ast_node* node, isl_ast_build* build, void* user) {
  isl::set iterator_domain(isl_ast_node_get_ctx(node), "{S0[i,j]: 0 < i < j < 100 }");
  ExtractIslTransformedIndiceMap(iterator_domain, build);
}

}  // namespace cinn
