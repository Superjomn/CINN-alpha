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

  isl_ctx* ctx() { return ctx_; }
  const isl::set& iterator_domain() { return iter_domain_; }

  void set_name(const std::string& name);

  void ApplyTransformationOnScheduleRange(const std::string& map_str);

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

 private:
  // Init schedule with identity schedule.
  void InitSchedule();

  void InitFromExpr(Expr x);
};

}  // namespace cinn
