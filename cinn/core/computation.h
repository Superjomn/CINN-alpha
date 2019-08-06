#include <cinn/ir/ir.h>
#include "isl/map.h"
#include "isl/set.h"

namespace cinn {

class Computation {
  // ISL context.
  isl_ctx* ctx_;

  // Iteration domain.
  isl_set* iter_domain_;

  ir::Expr expr_;

  // The schedules of this computation.
  isl_map* schedule_;

  // Name of this computation.
  std::string name_;

 public:
  Computation() : ctx_(isl_ctx_alloc()) {}

  isl_ctx* ctx() { return ctx_; }

  /*
   * Apply a transformation on the domain of the schedule.
   */
  void ApplyTransformationOnScheduleDomain(const std::string& map_str) {
    CHECK(ctx_);
    isl_map* map = isl_map_read_from_str(ctx_, map_str.c_str());
    CHECK(map) << "map parse failed, " << map_str;

    auto* scheduled = isl_map_apply_domain(isl_map_copy(schedule_), isl_map_copy(map));
    SetSchedule(scheduled);
    VLOG(2) << "schedule: " << isl_map_to_str(schedule_);
  }

  void AddScheduleConstraint(const std::string& domain_constraints, const std::string& range_constraints) {
    CHECK(ctx_);
    isl_map* sched = schedule_;

    if (!domain_constraints.empty()) {
      isl_set* domain_crts = isl_set_read_from_str(ctx_, domain_constraints.c_str());
      CHECK(domain_crts);
      sched = isl_map_intersect_domain(isl_map_copy(sched), isl_set_copy(domain_crts));
    }

    if (!range_constraints.empty()) {
      isl_set* range_crts = isl_set_read_from_str(ctx_, range_constraints.c_str());
      sched = isl_map_intersect_range(isl_map_copy(sched), isl_set_copy(range_crts));
    }

    SetSchedule(sched);
  }

  void AssertNamesNotAssigned(const std::vector<std::string>& dimensions) {
    for (const auto& dim : dimensions) {
      auto id = isl_map_find_dim_by_name(schedule_, isl_dim_in, dim.c_str());
      CHECK_LT(id, 0);
      id = isl_map_find_dim_by_name(schedule_, isl_dim_out, dim.c_str());
      CHECK_LT(id, 0);
    }
  }

  /*
   * Set the computation's schedule.
   */
  void SetSchedule(isl_map* x) {
    CHECK(x);
    schedule_ = x;
  }
};

}  // namespace cinn
