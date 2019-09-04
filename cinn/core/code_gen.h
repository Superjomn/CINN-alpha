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

isl_ast_expr* CreateIslAstIndexExpression(isl_ast_build* build, const isl::map& access);

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
std::map<std::string, isl_ast_expr*> ExtractIslTransformedIndiceMap(const isl::set& iterator_domain,
                                                                    isl_ast_build* build);

static isl_ast_node* node_info_collector(isl_ast_node* node, isl_ast_build* build, void* user) {
  isl::set iterator_domain(isl_ast_node_get_ctx(node), "{S0[i,j]: 0 < i < j < 100 }");
  ExtractIslTransformedIndiceMap(iterator_domain, build);
}

}  // namespace cinn
