#pragma once
#include <cinn/ir/ir.h>
#include <glog/logging.h>
#include <isl/ast.h>
#include <isl/cpp.h>
#include <isl/space.h>
#include <map>
#include "cinn/core/stage.h"

namespace cinn {

class Generator;

// Transform ISL AST expr to CINN expression.
void IslAstExprToCinnExpr(isl_ast_expr* node, ir::Expr* expr);

// Transform ISL AST node to CINN expression.
void IslAstNodeToCinnExpr(isl_ast_node* node, cinn::ir::Expr* expr);

void GenCodeFromIslAst(isl_ast_node* node);

isl::ast_expr CreateIslAstIndexExpression(isl_ast_build* build, const isl::map& access);

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
std::map<std::string, isl::ast_expr> ExtractIslTransformedIndiceMap(const isl::set& iterator_domain,
                                                                    isl_ast_build* build);

isl_ast_node* IslAstNodeInfoCollect(isl_ast_node* node, isl_ast_build* build, void* user);

void ReplaceCinnIndiceWithIslTransformedIndicesHelper(const std::map<std::string, Expr>& indice_map, Expr& root);
Expr ReplaceCinnIndiceWithIslTransformedIndices(const std::map<std::string, isl::ast_expr>& indice_map, Expr& root);

//! Replace the Reference expression in the original_expr to `expr`.
void ReplaceExprWithStage(Expr& origin_expr, const std::string& s, const Expr& expr);

class Stage;

/**
 * @brief Base class of all the generators.
 *
 * Generator is an resource manager across the whole thread concerning the code generation phase.
 */
class Generator {
 public:
  static Generator& Global();

  // the isl ctx accross the whole thread.
  isl::ctx& ctx() { return ctx_; }

  //! Get a stage by its name.
  Stage* GetStageByName(const std::string& name);
  //! Register a stage.
  void RegisterStage(const std::string& name, Stage* x);
  //! Given a domain, collect all the stages those iterator domains intersect it.
  std::vector<Stage*> FilterStagesByDomain(const isl::set& domain);

  Stage* GetComputationByNode(isl_ast_node* node);

  size_t num_stages() const { return stages_.size(); }

 private:
  Generator() : ctx_(isl_ctx_alloc()) {}

  isl::ctx ctx_{nullptr};
  std::map<std::string, Stage*> stages_;
};

}  // namespace cinn
