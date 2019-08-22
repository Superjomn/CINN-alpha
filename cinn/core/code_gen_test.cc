#include "cinn/core/code_gen.h"
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/union_set.h>

namespace cinn {

TEST(code_gen, CinnExprFromIslAstExpr) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t,i+t*10]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));

  auto *builder = isl_ast_build_from_context(context);
  auto *ast = isl_ast_build_node_from_schedule_map(builder, T);

  isl_ast_expr *iter = isl_ast_node_for_get_iterator(ast);

  ir::Expr expr;
  CinnExprFromIslAstExpr(iter, &expr);

  LOG(INFO) << "Get node type " << static_cast<int>(expr.type());
}

}  // namespace cinn