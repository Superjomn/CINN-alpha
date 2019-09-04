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
#include "cinn/ir/ops_overload.h"

namespace cinn {

using namespace ir;

TEST(code_gen, IslAstExprToCinnExpr) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t,i+t*10]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));

  auto *builder = isl_ast_build_from_context(context);

  // set iterators
  isl_id_list *iterators = isl_id_list_alloc(ctx, 2);
  isl_id *id = isl_id_alloc(ctx, "t", nullptr);
  iterators = isl_id_list_add(iterators, id);
  id = isl_id_alloc(ctx, "i", nullptr);
  iterators = isl_id_list_add(iterators, id);
  builder = isl_ast_build_set_iterators(builder, iterators);

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

TEST(code_gen, ExtractIslTransformedIndiceMap) {
  Var i("i", 0, 100);
  Var j("j", 0, 200);

  Expr A("A"), B("B");

  Stage s0 = B(i, j).Assign(A(i, j) + 1);

  isl::set context(s0.ctx(), "{:}");
  auto *build = isl_ast_build_from_context(context.copy());

  LOG(INFO) << "iterator_domain: " << s0.iterator_domain();
  isl::map transform(s0.ctx(), "[T,N] -> {S0[i,j] -> [i-1,j+1]}");
  isl::union_map T =
      isl::manage(isl_union_map_from_map(isl_map_intersect_domain(transform.copy(), s0.iterator_domain().copy())));
  LOG(INFO) << "T: " << T;

  isl_ast_build_set_at_each_domain(build, node_info_collector, nullptr);

  auto *ast = isl_ast_build_node_from_schedule_map(build, T.copy());

  LOG(INFO) << "\n" << isl_ast_node_to_C_str(ast);
}

}  // namespace cinn