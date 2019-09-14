#pragma once
#include <cinn/ir/ir.h>
#include <gtest/gtest_prod.h>
#include <isl/aff.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/cpp.h>
#include <isl/ctx.h>
#include <isl/id.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <map>
#include <sstream>

#include "cinn/core/buffer.h"

namespace cinn {
using ir::Expr;

/**
 * Stage is an statement.
 */
class Stage {
  struct StageData {
    // ISL context.
    isl_ctx* ctx{};

    // Iteration domain.
    isl::set iter_domain;

    ir::Expr expr;

    // The schedules of this computation.
    isl_utils::map schedule;

    // Name of this computation.
    std::string name;

    // statements' indice map from CINN to isl ast.
    std::map<std::string, Expr> indice_map_;

    static std::set<std::string> names;
  };

  std::shared_ptr<StageData> data_;

 public:
  Stage() { InitData(); }
  Stage(const std::string& name, const std::string& iter_domain);

  /**
   * Create a Stage using an Expr (should be an Reference node).
   *
   * For example
   *
   *     Var i("i", 0, 100);
   *     Expr A, B;
   *     B(i) = A(i) + 1;
   *
   *     Stage s0(B);
   */
  Stage(Expr expr);

  // Stage is free to copy.
  Stage(const Stage& stage) : data_(stage.data_) {}
  Stage(Stage&& stage) : data_(std::move(stage.data_)) {}

  void operator=(const Stage& other) { data_ = other.data_; }

  /// Get the expression this stage holds.
  const Expr& expr() const { return data_->expr; }

  //! Tell whether this stage is an Assign stage.
  bool is_assign() const;
  //! Tell whether this stage is an Allocate stage.
  bool is_allocate() const;

  //! Get the isl ctx.
  isl_ctx* ctx() { return data_->ctx; }
  //! Get the iteration domain of the expression this stage holds.
  const isl::set& iterator_domain() const { return data_->iter_domain; }

  //! Set name of the stage.
  void SetName(const std::string& name);
  //! Get the name of the stage.
  const std::string& name() const { return data_->name; }

  const isl_utils::map& schedule() const { return data_->schedule; }

  // Some basic polyhedral transformations

  //! Interchange two loop levels `i` and `j`.
  void Interchange(ir::Var i, ir::Var j);

  //! Interchange two loop levels named `i` and `j`.
  void Interchange(const std::string& dim0, const std::string& dim1);

  //! Tile in `i` iterator by `iw` stride, `j` iterator by `jw` stride.
  void Tile(ir::Var i, size_t iw, ir::Var j, size_t jw);

  //! Skew in the loop level `i`.
  void Skew(ir::Var i);

  //! Reverse the loop level `i`.
  void Reverse(ir::Var i);

  //! Vectorize the loop level `i`.
  void Vectorize(ir::Var i, size_t vec_size);

  // After transformations.

  void ApplyTransformationOnScheduleRange(const std::string& map_str);

  isl::map GetTransformedSchedule();

  Stage& operator=(Expr x) {
    InitFromAssignExpr(x);
    return *this;
  }

  // Dump the schedule to ISL C code.
  std::string DumpIslC() const;

  // Dump to C-like code.
  std::string DumpAsC() const;

  void SetIndiceMap(std::map<std::string, Expr>&& indice_map) { data_->indice_map_ = std::move(indice_map); }
  const std::map<std::string, Expr>& indice_map() const { return data_->indice_map_; }

  Expr GetIndiceTransformedExpr() const;

 private:
  void InitData();
  // Init schedule with identity schedule.
  void InitSchedule();
  //! Name all the dimensions of the schedule if not named.
  void ScheduleNameAllDims();
  //! Initialize the stage from an assign expression, It will extract the domain and schedule information from the
  //! expression.
  void InitFromAssignExpr(Expr x);
  //! Initialize the stage from an allocate expression.
  void InitFromAllocateExpr(Expr x);
  //! Interchange the i-th and j-th loop level.
  void Interchange(int pos0, int pos1);

  // Tests
  FRIEND_TEST(Stage, Interchange);
};

}  // namespace cinn
