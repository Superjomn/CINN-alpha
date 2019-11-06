#pragma once
#include <gtest/gtest_prod.h>
#include <isl/cpp.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/buffer.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/isl_utils.h"

namespace cinn {
using ir::Expr;

/**
 * \brief Snippet is a list of Stages can merged into a single polyhedral representation or a non-inline function
 * call.
 *
 * For example, if we have following list of stages in order:
 *
 * {
 * S0: A[i][j] = B[i][j] * 2
 * S1: C[i][j] = 1.
 * S2: tanh(A)
 * S3: B[i][j] = max(B[i][j], 0)
 * }
 *
 * It will output three Snippets:
 *
 * the first is a polyhedral snippet: {S0, S1},
 * the second is a function call that can't transform to a polyhedral model,
 * the third is another polyhedral snippet {S3}
 */
class Snippet {
 public:
  Snippet()
      : iterator_domain_(new isl::union_set),
        transform_(new isl::union_map),
        access_reads_(new isl::union_map),
        access_writes_(new isl::union_map),
        memory_dependencies_(new isl::union_map),
        schedule_(new isl::schedule),
        ctx_(isl_ctx_alloc()) {}

  //! Add a stage to snippet.
  void AddStage(const Stage& stage);

  //! End of snippet definition, should't AddStage latter.
  void End();

  Stage::Type type() const { return type_; }

  const isl::union_set& iterator_domain() const { return *iterator_domain_; }
  const isl::union_map& transform() const { return *transform_; }
  const isl::union_map& access_reads() const { return *access_reads_; }
  const isl::union_map& access_writes() const { return *access_writes_; }

  bool is_polyhedral() const { return Stage::is_polyhedral(type()); }
  bool is_function_call() const { return Stage::is_function_call(type()); }
  bool is_unk() const { return Stage::is_unk(type()); }

  //! Try to fuse two stages if possible.
  void TryFuse(const std::string& stage0, const std::string& stage1);

  Expr GetTransformedExpr() const;

 private:
  //! Collect the iterator domain from stages, only works for polyhedral snippets.
  void CollectIteratorDomain();

  //! Collect polyhedral transforms from stages.
  void CollectTransforms();

  //! Collect read access information from stages.
  void CollectReadAccess();

  //! Collect write access information from stages.
  void CollectWriteAccess();

  //! Compute the polyhedral schedule.
  void ComputeSchedule();

  //! Generate the isl ast.
  isl::ast_node GenerateIslAst() const;

  //! Tile the stages.
  void ApplyTiles();

  //! Apply the transpose transformations to the schedule.
  void ApplyTransposes();

  void ApplyVectorize();

  //! Fuse the stages if set with Stage::FuseWith.
  void BuildFusion();

 private:
  //! stages in order.
  std::vector<Stage> stages_;

  std::unique_ptr<isl::union_set> iterator_domain_;
  std::unique_ptr<isl::union_map> transform_;

  std::unique_ptr<isl::union_map> access_reads_;
  std::unique_ptr<isl::union_map> access_writes_;

  std::unique_ptr<isl::union_map> memory_dependencies_;

  // Config the stages that might be fuse together.
  std::unique_ptr<isl::union_map> approxi_;

  std::unique_ptr<isl::schedule> schedule_;

  Stage::Type type_{Stage::Type::unk};

  isl::ctx ctx_;

  mutable bool is_end_{false};
};

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
struct Function {
  struct Data {
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

    bool is_inline{false};
    isl::union_set iterator_domain;
    std::map<std::string, isl_utils::map> schedules;

    std::vector<Snippet> snippets;

    //! the final compiled expr.
    Expr transformed_expr;

    //! the final ir representation.
    Expr ir_function;

    isl_ctx* ctx{nullptr};

    bool end_definition{false};
  };

 private:
  std::shared_ptr<Data> data_;

 public:
  Function() {
    InitData();
    data_->name = NameGenerator::Global().NewFuncionName();
  }
  //! Create a function with name specified, the other member should be inferenced latter.
  Function(const std::string& name) {
    InitData();
    data_->name = name;
    data_->ctx = isl_utils::global_isl_ctx();
  }
  Function(const Function& other) { data_ = other.data_; }

  /**
   * Add a stage to this Function.
   * @param stage the stage to add.
   * @return the added stage.
   */
  Stage AddStage(const Stage& stage);

  /**
   * Try to fuse a and b stages, it will fuse them if possible.
   */
  void Fuse(const Stage& a, const Stage& b);

  /**
   * Set the function's inputs.
   */
  void Inputs(const std::vector<Expr>& xs) { data_->inputs = xs; }

  //! Set the function's outupts.
  void Outputs(const std::vector<Expr>& xs) { data_->outputs = xs; }

  Expr operator()(const std::vector<Expr>& inputs, const std::vector<Expr>& outputs);

  const std::vector<Snippet>& snippets() const { return data_->snippets; }
  std::vector<Snippet>* mutable_snippets() { return &data_->snippets; }

  const std::string& name() const { return data_->name; }
  const std::vector<ir::Expr>& inputs() const { return data_->inputs; }
  const std::vector<ir::Expr>& outputs() const { return data_->outputs; }
  const std::vector<Stage>& stages() const { return data_->stages; }
  std::vector<Stage>& stages() { return data_->stages; }
  const isl::schedule& schedule() const { return data_->schedule; }
  isl_ctx* ctx() { return data_->ctx; }
  bool end_definition() const { return data_->end_definition; }

  //! Define a function.
  static std::shared_ptr<Function> make(const std::string& name,
                                        std::vector<Expr> inputs,
                                        std::vector<Expr> outputs,
                                        std::vector<Stage> stages);

  //! Mark the function inline.
  void set_inline() { data_->is_inline = true; }
  //! Tell whether this function is an inline one.
  bool is_inline() const { return data_->is_inline; }

  const Expr& ComputeTransformedExpr() const;

  /**
   * Return the corresponding ir::Function.
   */
  const ir::Expr& ir_function() const { return data_->ir_function; }

  operator Expr();

  // TODO(Superjomn) Remove this method
  /**
   * Finalize the definition of a Function, new stages cann't add to this function latter.
   */
  void EndDefinition() {
    data_->transformed_expr = Expr();
    BuildSnippets();
    data_->ir_function = ir::Function::make(name(), data_->inputs, data_->outputs, ComputeTransformedExpr());
    data_->end_definition = true;
  }

  /**
   * Clear the build definitions, only the stages is kept, the stages and computed ir are cleard.
   */
  void ResetDefintion() {
    data_->snippets.clear();
    data_->ir_function = ir::Expr();
    data_->end_definition = false;
  }

  void BuildSnippets();

  // void PreAppendStage(const Stage& stage);

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
  void InitData() {
    CHECK(!data_);
    data_ = std::make_shared<Function::Data>();
    data_->ctx = isl_utils::global_isl_ctx();
  }

  FRIEND_TEST(cpp_code_gen, basic);
};

}  // namespace cinn
