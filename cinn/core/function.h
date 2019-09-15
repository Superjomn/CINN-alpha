#pragma once
#include <gtest/gtest_prod.h>
#include <isl/cpp.h>
#include <map>
#include <string>
#include "cinn/core/buffer.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/isl_utils.h"

namespace cinn {
using ir::Expr;

struct Stage;
/**
 * Function is the core part of CINN.
 *
 * Definition of a Function:
 *
 *     Function func({inputs}, {outputs})
 *
 * We make both the inputs and outputs a list, so that if multiple exprs relays on the same temporarily values, they
 * can be composed in a same function.
 *
 * Call a Function:
 *
 *     Expr A, B, C;
 *     std::vector<Expr> outs = func({B, C});
 *
 * Usage:
 *
 *     Var i("i", 0, 100),j("j", 0, 150);
 *     Expr A, B, C;
 *     Stage s0 = A(i,j).Assign( B(i,j) + C(i,j) );
 *
 *     Function func0({B,C}, {A}, {s0});
 *     func0.set_inline();
 *     // func0.dims(), get dimension of the cumputation.
 *
 *     Expr B0, C0;
 *     Expr A0 = func0(B0,C0)[0];
 *     // will expand to
 *     Expr A0;
 *     A0(i,j) = B(i,j) + C(i,j)
 */
struct Function : public ir::ExprNode<Function> {
  //! Name of the function, should be unique across the compile context.
  std::string name;

  //! Pass argument by value.
  std::vector<Buffer*> arguments;

  //! For inline function to expand the definition inplace.
  std::vector<ir::Expr> inputs;

  //! For inline function to expand the definition inplace.
  std::vector<ir::Expr> outputs;

  //! Body of the function.
  std::vector<Stage> stages;

  //! All of the dependencies.
  isl::union_map dependencies;

  //! Schedule of the stages.
  // It will compute automatically by the dependence of the stages.
  isl::schedule schedule;

  //! Define a function.
  static std::shared_ptr<Function> make(const std::string& name,
                                        std::vector<Expr> inputs,
                                        std::vector<Expr> outputs,
                                        std::vector<Stage> stages);

  //! Mark the function inline.
  void set_inline() { is_inline_ = true; }
  //! Tell whether this function is an inline one.
  bool is_inline() const { return is_inline_; }

  Expr GetTransformedExpr() const;

  //! Dump C like codes.
  std::string DumpIslC() const;

  std::string Dump() const;

  void Accept(ir::IRVisitor* visitor) const override;

  static const ir::NodeTy node_type = ir::NodeTy::Function;

  const isl::union_set& iterator_domain() const { return iterator_domain_; }

  isl::union_map GetFinalTransform() const;

  Function();

  void PreAppendStage(const Stage& stage);

  //! Compute the dependence relations between the stages, we treat the WAR, WAW, RAW as dependencies.
  // For example:
  // for (i=0; i<10; i++)
  //   A[i] = 0;           // S0
  // for (i=0; i<20; i++)
  //   for (j=0; j<30; j++)
  //     B[i,j] += A[i];   // S1
  // And we will get { S1[i,j] -> S0[a] }
  void ComputeStageFlows();

  //! Compute the schedule.
  void ComputeSchedule();

 protected:
  //! Schedule the stages by their original order.
  void CollectIteratorDomain();
  //! Initialize the schedule to identity for each stage.
  void InitSchedule();
  //! Generate the final ISL ast node.
  isl::ast_node GenerateIslAst() const;

  FRIEND_TEST(cpp_code_gen, basic);

 private:
  bool is_inline_{false};
  isl::ctx ctx_;
  isl::union_set iterator_domain_;
  std::map<std::string, isl_utils::map> schedule_;
  // isl::union_map schedule_;
};

}  // namespace cinn
