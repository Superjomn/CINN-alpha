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

isl_map* isl_map_add_dim_and_eq_constraint(isl_map* map, int dim_pos, int constant) {
  CHECK(map != NULL);
  CHECK(dim_pos >= 0);
  CHECK(dim_pos <= (signed int)isl_map_dim(map, isl_dim_out));

  map = isl_map_insert_dims(map, isl_dim_out, dim_pos, 1);
  map = isl_map_set_tuple_name(map, isl_dim_out, isl_map_get_tuple_name(map, isl_dim_in));

  isl_space* sp = isl_map_get_space(map);
  isl_local_space* lsp = isl_local_space_from_space(isl_space_copy(sp));
  isl_constraint* cst = isl_constraint_alloc_equality(lsp);
  cst = isl_constraint_set_coefficient_si(cst, isl_dim_out, dim_pos, 1);
  cst = isl_constraint_set_constant_si(cst, (-1) * constant);
  map = isl_map_add_constraint(map, cst);

  return map;
}

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
  Computation(const std::string& name, const std::string& iter_domain) : ctx_(isl_ctx_alloc()) {
    CHECK(ctx_);
    iter_domain_ = isl_set_read_from_str(ctx_, iter_domain.c_str());
    CHECK(iter_domain_);
    InitSchedule();
  }

  void SetName(const std::string& name) {
    CHECK(!name.empty());
    CHECK(!names_.count(name)) << "duplicate name for Computation, " << name;
    name_ = name;
    names_.insert(name_);
  }

  /*
   * Apply a transformation on the domain of the schedule.
   */
  void ApplyTransformationOnScheduleDomain(const std::string& map_str) {
    CHECK(ctx_);
    isl_map* map = isl_map_read_from_str(ctx_, map_str.c_str());
    CHECK(map) << "map parse failed, " << map_str;

    LOG(INFO) << "map " << map_str;
    auto* scheduled = isl_map_apply_domain(isl_map_copy(schedule_), isl_map_copy(map));
    LOG(INFO) << "apply transformation on schedule domain: "
              << isl_map_to_str(isl_map_intersect_domain(isl_map_copy(schedule_), isl_set_copy(iter_domain_)));
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

  // Dump the schedule to human-readable code.
  std::string Dump() const {
    std::stringstream ss;
    CHECK(schedule_);
    CHECK(iter_domain_);
    CHECK(ctx_);
    auto* T = isl_union_map_from_map(isl_map_intersect_domain(schedule_, iter_domain_));
    auto* C = isl_set_read_from_str(ctx_, "{:}");
    auto* build = isl_ast_build_from_context(C);
    auto* ast = isl_ast_build_node_from_schedule_map(build, T);
    ss << "generated code:\n" << isl_ast_node_to_C_str(ast);
  }

  /*
   * Set the computation's schedule.
   */
  void SetSchedule(isl_map* x) {
    CHECK(x);
    schedule_ = x;
  }

 private:
  // Init schedule with identity schedule.
  void InitSchedule() {
    CHECK(iter_domain_);
    isl_space* space = isl_set_get_space(iter_domain_);
    isl_map* schedule = isl_map_identity(isl_space_map_from_set(space));
    VLOG(2) << "identity schedule: " << isl_map_to_str(schedule);
    schedule = isl_map_intersect_domain(schedule, isl_set_copy(iter_domain_));
    schedule = isl_map_coalesce(schedule);

    for (int i = 0; i < isl_space_dim(space, isl_dim_out); i++) {
      schedule = isl_map_add_dim_and_eq_constraint(schedule, 2 * i, 0);
    }

    SetSchedule(schedule);

    LOG(INFO) << "after init: " << isl_map_to_str(schedule_);
    LOG(INFO) << "schedule: "
              << isl_map_to_str(isl_map_intersect_domain(isl_map_copy(schedule), isl_set_copy(iter_domain_)));
  }
};

}  // namespace cinn
