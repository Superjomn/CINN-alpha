#include "cinn/core/stage.h"
#include <isl/map.h>
#include <isl/set.h>
#include <set>
#include <string>
#include <vector>
#include "cinn/core/isl_code_gen.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/name_generator.h"
#include "cinn/utils/string.h"
#include "stage.h"

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
  data_->expr = expr;
  if (expr.is_assign()) {
    InitFromAssignExpr(expr);
  } else if (expr.is_allocate()) {
    InitFromAllocateExpr(expr);
  }

  Generator::Global().RegisterStage(data_->name, this);
}

void Stage::InitFromAssignExpr(Expr expr) {
  CHECK(!data_->ctx);
  CHECK(expr.valid());
  CHECK(expr.is_op() || expr.is_allocate());
  CHECK(!data_->iter_domain.get());
  CHECK(!data_->schedule.get());

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

std::string Stage::DumpIslC() const {
  LOG_INDENT("Stage::DumpIslC");
  std::stringstream ss;
  CHECK(data_->ctx);
  CHECK(data_->iter_domain.get());
  isl::map schedule;
  schedule = data_->schedule.get() ? data_->schedule : isl::manage(isl_set_to_identity_map(data_->iter_domain.copy()));
  isl::map t0 = isl::manage(isl_map_intersect_domain(schedule.copy(), data_->iter_domain.copy()));
  isl::set C(data_->ctx, "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));

  // rename iterators
  std::vector<std::string> iterators;
  int transform_range_dims = isl_map_dim(t0.get(), isl_dim_out);
  CINN_DEBUG(2) << "transform: " << t0;
  for (int i = 0; i < transform_range_dims; i++) {
    LOG(INFO) << "t0: " << t0 << isl_map_has_dim_name(t0.get(), isl_dim_out, i);
    if (isl_bool_true == isl_map_has_dim_name(t0.get(), isl_dim_out, i)) {
      iterators.push_back(isl_map_get_dim_name(t0.get(), isl_dim_out, i));
    } else {
      iterators.push_back(NameGenerator::Global().NewIteratorName());
    }
  }

  build = isl::manage(isl_utils::isl_ast_build_set_iterators(build.release(), iterators));

  isl::union_map transform = isl::manage(isl_union_map_from_map(t0.copy()));
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule_map(build.get(), transform.release()));
  ss << isl_ast_node_to_C_str(ast.get());
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
  isl_utils::map schedule = data_->iter_domain.identity();
  // schedule = isl::manage(isl_map_set_tuple_name(schedule.release(), isl_dim_out, ""));
  CINN_DEBUG(2) << "schedule: " << schedule;

  schedule = isl::manage(isl_map_coalesce(schedule.release()));
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
  for (auto& item : data_->indice_map_) {
    CINN_DEBUG(3) << "dic " << item.first << " -> " << ir::Dump(item.second);
  }
  CHECK(data_->expr.valid());
  CHECK(!data_->indice_map_.empty()) << "indice_map should be created first";
  CINN_DEBUG(3) << "original expr: " << ir::Dump(data_->expr);
  Expr expr_copied = ir::CopyExpr(data_->expr);
  ReplaceCinnIndiceWithIslTransformedIndicesHelper(data_->indice_map_, expr_copied);
  return expr_copied;
}

void Stage::InitData() {
  CHECK(!data_);
  data_ = std::make_shared<StageData>();
  data_->ctx = Generator::Global().ctx().get();
  data_ = std::make_shared<StageData>();
}

isl::map Stage::GetTransformedSchedule() {
  CHECK(!data_->iter_domain.is_null());
  CHECK(!data_->schedule.is_null());
  return data_->schedule.intersect_domain(data_->iter_domain);
}

void Stage::InitFromAllocateExpr(Expr x) {}

bool Stage::is_assign() const { return expr().is_assign(); }

bool Stage::is_allocate() const { return expr().is_allocate(); }

std::set<std::string> Stage::StageData::names;

void Stage::Interchange(ir::Var i, ir::Var j) { Interchange(i.name(), j.name()); }

void Stage::Interchange(const std::string& dim0, const std::string& dim1) {
  // find the dimension position
  int pos0 = isl_map_find_dim_by_name(schedule().get(), isl_dim_out, dim0.c_str());
  int pos1 = isl_map_find_dim_by_name(schedule().get(), isl_dim_out, dim1.c_str());
  CHECK_NE(pos0, -1);
  CHECK_NE(pos1, -1);
  Interchange(pos0, pos1);
}

void Stage::Interchange(int pos0, int pos1) {
  LOG_INDENT("Stage::Interchange");
  ScheduleNameAllDims();

  int ndim = schedule().range_dims();
  CHECK_LT(pos0, ndim);
  CHECK_LT(pos1, ndim);

  const char* dim0_name = schedule().range_dim_name(pos0);
  const char* dim1_name = schedule().range_dim_name(pos1);

  std::vector<std::string> from_dims;
  std::vector<std::string> to_dims;
  for (int i = 0; i < ndim; i++) {
    from_dims.push_back(schedule().range_dim_name(i));
    if (i == pos0) {
      to_dims.push_back(dim1_name);
    } else if (i == pos1) {
      to_dims.push_back(dim0_name);
    } else {
      to_dims.push_back(schedule().range_dim_name(i));
    }
  }
  CHECK_EQ(from_dims.size(), to_dims.size());

  std::stringstream ss;
  ss << "{ " << name() << "[ " << Concat(from_dims, ", ") << " ]";
  ss << " -> ";
  ss << name() << "[ " << Concat(to_dims, ", ") << " ]"
     << " }";
  isl::map transform(ctx(), ss.str());

  // set dims

  for (int i = 0; i < isl_map_dim(transform.get(), isl_dim_out); i++) {
    transform = isl::manage(isl_map_set_dim_name(transform.release(), isl_dim_out, i, to_dims[i].c_str()));
  }

  CINN_DEBUG(2) << "transform: " << transform;
  data_->schedule = data_->schedule.apply_range(transform);
}

void Stage::ScheduleNameAllDims() {
  // Name all the range's dimensions.
  for (int i = 0; i < schedule().range_dims(); i++) {
    if (!schedule().range_has_dim_name(i)) {
      data_->schedule.range_set_dim_name(i, NameGenerator::Global().NewIteratorName().c_str());
    }
  }
}

}  // namespace cinn
