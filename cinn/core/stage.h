#pragma once
#include <cinn/ir/ir.h>
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
    isl::map schedule;

    // Name of this computation.
    std::string name;

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

  const Expr& expr() const { return data_->expr; }

  isl_ctx* ctx() { return data_->ctx; }
  const isl::set& iterator_domain() const { return data_->iter_domain; }

  void SetName(const std::string& name);
  const std::string& name() const { return data_->name; }

  void ApplyTransformationOnScheduleRange(const std::string& map_str);

  isl::map GetTransformedSchedule() {
    CHECK(!data_->iter_domain.is_null());
    CHECK(!data_->schedule.is_null());
    return data_->schedule.intersect_domain(data_->iter_domain);
  }

  /*
   * Apply a transformation on the domain of the schedule.
   */
  void ApplyTransformationOnScheduleDomain(const std::string& map_str);

  void AddScheduleConstraint(const std::string& domain_constraints, const std::string& range_constraints);

  void AssertNamesNotAssigned(const std::vector<std::string>& dimensions);

  Stage& operator=(Expr x) {
    InitFromExpr(x);
    return *this;
  }

  // Dump the schedule to ISL C code.
  std::string DumpIslC() const;

  // Dump to C-like code.
  std::string DumpAsC() const;

  void SetIndiceMap(std::map<std::string, Expr>&& indice_map) { indice_map_ = std::move(indice_map); }
  const std::map<std::string, Expr>& indice_map() const { return indice_map_; }

  Expr GetIndiceTransformedExpr() const;

 private:
  void InitData();
  // Init schedule with identity schedule.
  void InitSchedule();

  void InitFromExpr(Expr x);

  std::map<std::string, Expr> indice_map_;
};

}  // namespace cinn
