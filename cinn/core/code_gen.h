#pragma once
#include <cinn/ir/ir.h>
#include <glog/logging.h>
#include <isl/ast.h>

namespace cinn {

// Transform ISL AST expr to CINN expression.
void IslAstExprToCinnExpr(isl_ast_expr *node, ir::Expr *expr);

// Transform ISL AST node to CINN expression.
void IslAstNodeToCinnExpr(isl_ast_node *node, cinn::ir::Expr *expr);

void GenCodeFromIslAst(isl_ast_node *node);

}  // namespace cinn
