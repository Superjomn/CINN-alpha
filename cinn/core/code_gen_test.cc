#include "cinn/core/code_gen.h"
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/union_set.h>
#include "cinn/ir/ir_printer.h"

namespace cinn {

TEST(code_gen, IslAstExprToCinnExpr) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t,i+t*10]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));

  auto *builder = isl_ast_build_from_context(context);
  auto *ast = isl_ast_build_node_from_schedule_map(builder, T);

  isl_ast_expr *iter = isl_ast_node_for_get_iterator(ast);

  // ISL print C code
  isl_printer *p = isl_printer_to_str(ctx);
  isl_printer_set_output_format(p, 0);
  isl_printer_print_ast_node(p, ast);
  LOG(INFO) << "\n" << isl_ast_node_to_C_str(ast);

  ir::Expr expr;
  IslAstExprToCinnExpr(iter, &expr);

  LOG(INFO) << "Get node type " << static_cast<int>(expr.type());

  std::stringstream os;
  ir::IRPrinter printer(os);

  ir::Expr for_expr;
  IslAstNodeToCinnExpr(ast, &for_expr);

  printer.Print(for_expr);
  LOG(INFO) << "\n" << os.str();
}

}  // namespace cinn