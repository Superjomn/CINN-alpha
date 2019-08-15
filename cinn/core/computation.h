#include <cinn/ir/ir.h>
#include <isl/aff.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/ctx.h>
#include <isl/id.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <sstream>

#include "cinn/core/buffer.h"

namespace cinn {

isl_map* isl_map_add_dim_and_eq_constraint(isl_map* map, int dim_pos, int constant);

class Computation {
  // ISL context.
  isl_ctx* ctx_{};

  // Iteration domain.
  isl_set* iter_domain_{};

  ir::Expr expr_;

  // The schedules of this computation.
  isl_map* schedule_{};

  // Name of this computation.
  std::string name_;

  static std::set<std::string> names_;

 public:
  isl_ctx* ctx() { return ctx_; }

  Computation() : ctx_(isl_ctx_alloc()) {}
  Computation(const std::string& name, const std::string& iter_domain);

  void SetName(const std::string& name);

  void ApplyTransformationOnScheduleRange(const std::string& map_str);

  /*
   * Apply a transformation on the domain of the schedule.
   */
  void ApplyTransformationOnScheduleDomain(const std::string& map_str);

  void AddScheduleConstraint(const std::string& domain_constraints, const std::string& range_constraints);

  void AssertNamesNotAssigned(const std::vector<std::string>& dimensions);

  // Dump the schedule to human-readable code.
  std::string Dump() const;

  /*
   * Set the computation's schedule.
   */
  void SetSchedule(isl_map* x);

 private:
  // Init schedule with identity schedule.
  void InitSchedule();
};

}  // namespace cinn
