#pragma once
#include <cinn/ir/ir.h>
#include <glog/logging.h>
#include <isl/ast.h>

namespace cinn {

void CinnExprFromIslAstExpr(isl_ast_expr* node, ir::Expr* expr);

void WalkIslAst(isl_ast_node* node, cinn::ir::Expr* expr);

void GenCodeFromIslAst(isl_ast_node* node);

}  // namespace cinn
