#include "cinn/core/stage.h"
#include <cinn/utils/isl_utils.h>
#include <isl/map.h>
#include <isl/set.h>
#include "cinn/core/code_gen.h"
#include "cinn/utils/name_generator.h"

namespace cinn {

isl::set ExtractDomainFromAssignExpr(const std::string& statement, Expr expr, isl_ctx* ctx) {
  CHECK(expr.type() == ir::NodeTy::Assign);
  auto assign = expr.As<ir::Assign>();
  auto& a = assign->a;
  CHECK(a.type() == ir::NodeTy::Reference);

  // Extract domain from the expr.
  CHECK(expr.type() == ir::NodeTy::Assign);
  auto ref = a.As<ir::Reference>();
  auto intervals = ref->ExtractIntervals();

  std::stringstream iterators_repr;
  for (int i = 0; i < ref->iterators.size() - 1; i++) {
    iterators_repr << ref->iterators[i].name() << ",";
  }
  if (ref->iterators.size() > 1) iterators_repr << ref->iterators.back().name();

  std::stringstream ss;

  ss << "{ " << statement << "[" << iterators_repr.str() << "] : ";
  auto iter_interval_repr = [&](ir::Var iter) {
    auto& interval = iter.interval();
    CHECK(interval.lower_bound().is_integer())
        << "polyhedral model can only support SCoP, the type of the iterator's domain should be integer";
    ss << interval.lower_bound().As<int32_t>() << " <= " << iter.name() << " < "
       << interval.upper_bound().As<int32_t>();
  };

  for (int i = 0; i < ref->iterators.size() - 1; i++) {
    iter_interval_repr(ref->iterators[i]);
    ss << " and ";
  }

  if (ref->iterators.size() > 1) {
    iter_interval_repr(ref->iterators.back());
  }
  ss << " }";

  isl::set uset(ctx, ss.str().c_str());
  VLOG(2) << "extract domain from expr: " << uset;
  return uset;
}

Stage::Stage(Expr expr) { InitFromExpr(expr); }

void Stage::InitFromExpr(Expr expr) {
  CHECK(!ctx_);
  CHECK(expr.valid());
  CHECK(expr.IsOp());
  CHECK(!iter_domain_.get());
  CHECK(!schedule_.get());
  ctx_ = isl_ctx_alloc();

  set_name(NameGenerator::Global().NewStageName());

  // set iterator_domain
  isl::set uset = ExtractDomainFromAssignExpr(name_, expr, ctx_);
  CHECK(uset.get()) << "failed to parse the expr to ISL domain";
  iter_domain_ = uset;

  // init schedule
  InitSchedule();
}

void Stage::ApplyTransformationOnScheduleRange(const std::string& map_str) {
  CHECK(ctx_);
  CHECK(schedule_.get());
  isl::map map(ctx_, map_str.c_str());
  CHECK(map.get()) << "map parse failed";

  LOG(INFO) << "schedule space " << schedule_.space();
  LOG(INFO) << "map space " << map.space();

  schedule_ = schedule_.apply_range(map);
}

void Stage::ApplyTransformationOnScheduleDomain(const std::string& map_str) {
  CHECK(ctx_);
  LOG(INFO) << "map " << map_str;
  isl::map map(ctx_, map_str.c_str());
  CHECK(map.get()) << "map parse failed, " << map_str;
  CHECK(schedule_.get());

  auto* scheduled = isl_map_apply_domain(schedule_.copy(), map.copy());
  schedule_ = isl::manage(scheduled);
  VLOG(2) << "schedule: " << schedule_;
}

void Stage::AddScheduleConstraint(const std::string& domain_constraints, const std::string& range_constraints) {
  CHECK(ctx_);
  isl::map sched = schedule_;

  if (!domain_constraints.empty()) {
    isl::set domain_crts(ctx_, domain_constraints.c_str());
    CHECK(domain_crts.get());
    sched = sched.intersect_domain(domain_crts);
  }

  if (!range_constraints.empty()) {
    isl::set range_crts(ctx_, range_constraints.c_str());
    sched = sched.intersect_range(range_crts);
  }

  schedule_ = sched;
}

void Stage::AssertNamesNotAssigned(const std::vector<std::string>& dimensions) {
  CHECK(schedule_.get());
  for (const auto& dim : dimensions) {
    auto id = isl_map_find_dim_by_name(schedule_.get(), isl_dim_in, dim.c_str());
    CHECK_LT(id, 0);
    id = isl_map_find_dim_by_name(schedule_.get(), isl_dim_out, dim.c_str());
    CHECK_LT(id, 0);
  }
}

std::string Stage::DumpIslC() const {
  std::stringstream ss;
  CHECK(ctx_);
  CHECK(iter_domain_.get());
  isl::map schedule;
  schedule = schedule_.get() ? schedule_ : isl::manage(isl_set_to_identity_map(iter_domain_.copy()));
  isl::union_map transform =
      isl::manage(isl_union_map_from_map(isl_map_intersect_domain(schedule.copy(), iter_domain_.copy())));
  LOG(INFO) << "transform: " << transform;
  // auto* C = isl_set_empty(isl_space_copy(isl_union_map_get_space(T)));
  isl::set C(ctx_, "{:}");
  auto* build = isl_ast_build_from_context(C.copy());
  auto* ast = isl_ast_build_node_from_schedule_map(build, transform.copy());
  ss << "generated code:\n" << isl_ast_node_to_C_str(ast);
  return ss.str();
}

std::string Stage::DumpAsC() const {
  CHECK(ctx_);
  CHECK(iter_domain_.get());
  isl::map schedule = schedule_.get() ? schedule_ : isl::manage(isl_set_to_identity_map(iter_domain_.get()));
  isl::union_map final =
      isl::manage(isl_union_map_from_map(isl_map_intersect_domain(schedule.copy(), iter_domain_.copy())));
  isl::set C(ctx_, "{:}");
  auto* build = isl_ast_build_from_context(C.copy());
  auto* ast = isl_ast_build_node_from_schedule_map(build, final.copy());
}

void Stage::InitSchedule() {
  CHECK(iter_domain_.get());
  isl::space space = iter_domain_.space();
  isl::map schedule = isl::manage(isl_map_identity(isl_space_map_from_set(space.copy())));
  VLOG(2) << "identity schedule: " << schedule;
  schedule = schedule.intersect_domain(iter_domain_);
  schedule = schedule.coalesce();

  for (int i = 0; i < isl_space_dim(space.get(), isl_dim_out); i++) {
    schedule = isl::manage(isl_map_add_dim_and_eq_constraint(schedule.copy(), 2 * i, 0));
  }

  schedule_ = schedule;

  LOG(INFO) << "after init: " << schedule_;
  LOG(INFO) << "schedule: " << isl::manage(isl_map_intersect_domain(schedule.copy(), iter_domain_.copy()));
}

Stage::Stage(const std::string& name, const std::string& iter_domain) : ctx_(isl_ctx_alloc()) {
  CHECK(ctx_);
  CHECK(!name.empty()) << "empty name found";
  CHECK(!iter_domain.empty()) << "empty iter_domain string found";
  iter_domain_ = isl::set(ctx_, iter_domain.c_str());
  CHECK(iter_domain_.get());
  InitSchedule();
}

void Stage::set_name(const std::string& name) {
  CHECK(!name.empty());
  CHECK(!names_.count(name)) << "duplicate name for Computation, " << name;
  name_ = name;
  names_.insert(name_);
}

std::set<std::string> Stage::names_;

}  // namespace cinn
