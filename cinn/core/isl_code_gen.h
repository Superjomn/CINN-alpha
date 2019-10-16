#pragma once
#include <glog/logging.h>
#include <isl/ast.h>
#include <isl/cpp.h>
#include <isl/space.h>
#include <map>
#include <string>
#include <vector>
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"

namespace cinn {

class Generator;

// Transform ISL AST expr to CINN expression.
void IslAstExprToCinnExpr(const isl::ast_expr& node, ir::Expr* expr);

// Transform ISL AST node to CINN expression.
void IslAstNodeToCinnExpr(const isl::ast_node& node, cinn::ir::Expr* expr);

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

/**
 * Attach CINN Reference with ISL forloop iterators.
 * We attach a stage each time.
 *
 * The steps:
 * 1. Find out the References corresponding to this stage.
 * 2. Extract the iterators of this references, currently just ISL iterators, e.g. S0[c0, c2+1] to [c0, c2+1]
 * 3. Map the CINN iterators to ISL iterators, e.g. S0[c0, c2+1]'s CINN expression is S0[i,j] = A[i][j] + 1, will get a
 * map: { i: c0, j:c2+1 }
 * 4. Replace the ISL iterators in Reference with the CINN expression: S0[c0,c2+1] -> A[c0][c2+1]+1
 *
 * @param root the CINN expression(or Reference with ISL iterators) to replace with.
 * @param statement the name of the stage.
 */
void AttachCinnExprToIslIndices(ir::Expr& root, const std::string& stage_name);

isl_ast_node* IslAstNodeInfoCollect(isl_ast_node* node, isl_ast_build* build, void* user);

void ReplaceCinnIndiceWithIslTransformedIndicesHelper(const std::map<std::string, ir::Expr>& indice_map,
                                                      ir::Expr& root);
ir::Expr ReplaceCinnIndiceWithIslTransformedIndices(const std::map<std::string, isl::ast_expr>& indice_map,
                                                    ir::Expr& root);

//! Replace the Reference expression in the original_expr to `expr`.
void ReplaceExprWithStage(ir::Expr& origin_expr, const std::string& s, const ir::Expr& expr);

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
  isl_ctx* ctx() { return isl_utils::global_isl_ctx(); }

  //! Get a stage by its name.
  Stage GetStageByName(const std::string& name);
  //! Register a stage.
  void RegisterStage(const std::string& name, Stage* x);
  //! Given a domain, collect all the stages those iterator domains intersect it.
  std::vector<Stage> FilterStagesByDomain(const isl::set& domain);

  Stage GetComputationByNode(isl_ast_node* node);

  size_t num_stages() const { return stages_.size(); }

 private:
  Generator() : ctx_(isl_ctx_alloc()) {}

  isl::ctx ctx_{nullptr};
  std::map<std::string, std::shared_ptr<Stage::Data>> stages_;
};

}  // namespace cinn
