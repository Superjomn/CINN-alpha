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
  // ISL context.
  isl_ctx* ctx_{};

  // Iteration domain.
  isl::set iter_domain_;

  ir::Expr expr_;

  // The schedules of this computation.
  isl::map schedule_;

  // Name of this computation.
  std::string name_;

  static std::set<std::string> names_;

 public:
  Stage() : ctx_(isl_ctx_alloc()) {}
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

  const Expr& expr() const { return expr_; }

  isl_ctx* ctx() { return ctx_; }
  const isl::set& iterator_domain() { return iter_domain_; }

  void SetName(const std::string& name);
  const std::string& name() const { return name_; }

  void ApplyTransformationOnScheduleRange(const std::string& map_str);

  isl::map GetTransformedSchedule() {
    CHECK(!iter_domain_.is_null());
    CHECK(!schedule_.is_null());
    return schedule_.intersect_domain(iter_domain_);
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

  Expr GetIndiceTransformedExpr();

 private:
  // Init schedule with identity schedule.
  void InitSchedule();

  void InitFromExpr(Expr x);

  std::map<std::string, Expr> indice_map_;
};

}  // namespace cinn
