#include "cinn/core/stage.h"
#include <isl/map.h>
#include <isl/set.h>
#include <set>
#include <string>
#include <vector>
#include "cinn/core/code_gen.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/name_generator.h"

namespace cinn {
using interval_tuple_t = ir::Reference::interval_tuple_t;

isl::set ExtractDomainFromAssignExpr(const std::string& statement, Expr expr, isl_ctx* ctx) {
  CINN_DEBUG(2) << statement << " extract domain";
  CINN_DEBUG(2) << "expr: " << ir::Dump(expr);

  CHECK(expr.type() == ir::NodeTy::Assign);
  auto assign = expr.As<ir::Assign>();
  auto& a = assign->a;
  CHECK(a.type() == ir::NodeTy::Reference);

  // Extract domain from the expr.
  CHECK(expr.type() == ir::NodeTy::Assign);
  auto ref = a.As<ir::Reference>();
  auto intervals = ref->ExtractIntervals();
  LOG(INFO) << "intervals: " << intervals.size();

  std::stringstream iterators_repr;
  for (int i = 0; i < intervals.size() - 1; i++) {
    iterators_repr << std::get<0>(intervals[i]) << ",";
  }
  if (intervals.size() >= 1) iterators_repr << std::get<0>(intervals.back());

  std::stringstream ss;

  ss << "{ " << statement << "[" << iterators_repr.str() << "] : ";
  auto iter_interval_repr = [&](const interval_tuple_t& iter) {
    std::string iter_name = std::get<0>(iter);
    auto& interval = std::get<1>(iter);
    CHECK(interval.lower_bound().is_integer())
        << "polyhedral model can only support SCoP, the type of the iterator's domain should be integer";
    ss << interval.lower_bound().As<int32_t>() << " <= " << iter_name << " < " << interval.upper_bound().As<int32_t>();
  };

  for (int i = 0; i < intervals.size() - 1; i++) {
    iter_interval_repr(intervals[i]);
    ss << " and ";
  }

  if (ref->iterators.size() >= 1) {
    iter_interval_repr(intervals.back());
  }
  ss << " }";

  isl::set uset(ctx, ss.str().c_str());
  VLOG(2) << "extract domain from expr: " << uset;
  return uset;
}

Stage::Stage(Expr expr) {
  InitData();
  InitFromExpr(expr);
  Generator::Global().RegisterStage(data_->name, this);
}

void Stage::InitFromExpr(Expr expr) {
  CHECK(!data_->ctx);
  CHECK(expr.valid());
  CHECK(expr.is_op());
  CHECK(!data_->iter_domain.get());
  CHECK(!data_->schedule.get());

  data_->expr = expr;

  data_->ctx = Generator::Global().ctx().get();
  SetName(NameGenerator::Global().NewStageName());

  // set iterator_domain
  isl::set uset = ExtractDomainFromAssignExpr(data_->name, expr, data_->ctx);
  CHECK(uset.get()) << "failed to parse the expr to ISL domain";
  data_->iter_domain = uset;

  // init schedule
  InitSchedule();
}

void Stage::ApplyTransformationOnScheduleRange(const std::string& map_str) {
  LOG_INDENT("Stage::ApplyTransformationOnScheduleRange");
  CHECK(data_->ctx);
  CHECK(data_->schedule.get());
  isl::map map(data_->ctx, map_str.c_str());
  CHECK(map.get()) << "map parse failed";

  CINN_DEBUG(2) << "schedule space " << data_->schedule.space();
  CINN_DEBUG(2) << "map space " << map.space();

  data_->schedule = data_->schedule.apply_range(map);
}

void Stage::ApplyTransformationOnScheduleDomain(const std::string& map_str) {
  CHECK(data_->ctx);
  LOG(INFO) << "map " << map_str;
  isl::map map(data_->ctx, map_str.c_str());
  CHECK(map.get()) << "map parse failed, " << map_str;
  CHECK(data_->schedule.get());

  auto* scheduled = isl_map_apply_domain(data_->schedule.copy(), map.copy());
  data_->schedule = isl::manage(scheduled);
  VLOG(2) << "schedule: " << data_->schedule;
}

void Stage::AddScheduleConstraint(const std::string& domain_constraints, const std::string& range_constraints) {
  CHECK(data_->ctx);
  isl::map sched = data_->schedule;

  if (!domain_constraints.empty()) {
    isl::set domain_crts(data_->ctx, domain_constraints.c_str());
    CHECK(domain_crts.get());
    sched = sched.intersect_domain(domain_crts);
  }

  if (!range_constraints.empty()) {
    isl::set range_crts(data_->ctx, range_constraints.c_str());
    sched = sched.intersect_range(range_crts);
  }

  data_->schedule = sched;
}

void Stage::AssertNamesNotAssigned(const std::vector<std::string>& dimensions) {
  CHECK(data_->schedule.get());
  for (const auto& dim : dimensions) {
    auto id = isl_map_find_dim_by_name(data_->schedule.get(), isl_dim_in, dim.c_str());
    CHECK_LT(id, 0);
    id = isl_map_find_dim_by_name(data_->schedule.get(), isl_dim_out, dim.c_str());
    CHECK_LT(id, 0);
  }
}

std::string Stage::DumpIslC() const {
  LOG_INDENT("Stage::DumpIslC");
  std::stringstream ss;
  CHECK(data_->ctx);
  CHECK(data_->iter_domain.get());
  isl::map schedule;
  schedule = data_->schedule.get() ? data_->schedule : isl::manage(isl_set_to_identity_map(data_->iter_domain.copy()));
  isl::union_map transform =
      isl::manage(isl_union_map_from_map(isl_map_intersect_domain(schedule.copy(), data_->iter_domain.copy())));
  CINN_DEBUG(2) << "transform: " << transform;
  // auto* C = isl_set_empty(isl_space_copy(isl_union_map_get_space(T)));
  isl::set C(data_->ctx, "{:}");
  auto* build = isl_ast_build_from_context(C.copy());
  auto* ast = isl_ast_build_node_from_schedule_map(build, transform.copy());
  ss << "generated code:\n" << isl_ast_node_to_C_str(ast);
  return ss.str();
}

std::string Stage::DumpAsC() const {
  CHECK(data_->ctx);
  CHECK(data_->iter_domain.get());
  isl::map schedule =
      data_->schedule.get() ? data_->schedule : isl::manage(isl_set_to_identity_map(data_->iter_domain.get()));
  isl::union_map final =
      isl::manage(isl_union_map_from_map(isl_map_intersect_domain(schedule.copy(), data_->iter_domain.copy())));
  isl::set C(data_->ctx, "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule_map(build.get(), final.copy()));

  Expr expr;
  IslAstNodeToCinnExpr(ast, &expr);
  return ir::Dump(expr);
}

void Stage::InitSchedule() {
  LOG_INDENT(data_->name + ".InitSchedule");
  CHECK(data_->iter_domain.get());
  /*
  isl::space space = iter_domain_.space();
  isl::map schedule = isl::manage(isl_map_identity(isl_space_map_from_set(space.copy())));
  CINN_DEBUG(2) << "identity schedule: " << schedule;
  schedule = schedule.intersect_domain(iter_domain_);
  schedule = schedule.coalesce();

  for (int i = 0; i < isl_space_dim(space.get(), isl_dim_out); i++) {
    schedule = isl::manage(isl_map_add_dim_and_eq_constraint(schedule.release(), 2 * i, 0));
  }
   */
  isl::map schedule = data_->iter_domain.identity();
  schedule = isl::manage(isl_map_set_tuple_name(schedule.release(), isl_dim_out, ""));
  CINN_DEBUG(2) << "schedule: " << schedule;

  data_->schedule = schedule;

  CINN_DEBUG(2) << "after init: " << data_->schedule;
  CINN_DEBUG(2) << data_->name
                << ".schedule: " << isl::manage(isl_map_intersect_domain(schedule.copy(), data_->iter_domain.copy()));
}

Stage::Stage(const std::string& name, const std::string& iter_domain) {
  InitData();
  CHECK(data_->ctx);
  CHECK(!name.empty()) << "empty name found";
  CHECK(!iter_domain.empty()) << "empty iter_domain string found";
  data_->iter_domain = isl::set(data_->ctx, iter_domain.c_str());
  CHECK(!data_->iter_domain.is_null());
  InitSchedule();

  Generator::Global().RegisterStage(data_->name, this);
}

void Stage::SetName(const std::string& name) {
  CHECK(!name.empty());
  CHECK(!data_->names.count(name)) << "duplicate name for Computation, " << name;
  data_->name = name;
  data_->names.insert(data_->name);
}

Expr Stage::GetIndiceTransformedExpr() const {
  LOG_INDENT("Stage::GetIndiceTransformedExpr");
  CINN_DEBUG(3) << "stage: " << name() << " : " << ir::Dump(data_->expr);
  for (auto& item : indice_map_) {
    CINN_DEBUG(3) << "dic " << item.first << " -> " << ir::Dump(item.second);
  }
  CHECK(data_->expr.valid());
  CHECK(!indice_map_.empty()) << "indice_map should be created first";
  CINN_DEBUG(3) << "original expr: " << ir::Dump(data_->expr);
  Expr expr_copied = ir::CopyExpr(data_->expr);
  ReplaceCinnIndiceWithIslTransformedIndicesHelper(indice_map_, expr_copied);
  return expr_copied;
}

void Stage::InitData() {
  CHECK(!data_);
  data_ = std::make_shared<StageData>();
  data_->ctx = Generator::Global().ctx().get();
  data_ = std::make_shared<StageData>();
}

std::set<std::string> Stage::StageData::names;

}  // namespace cinn
