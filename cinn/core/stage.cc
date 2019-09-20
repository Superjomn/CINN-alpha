#include "cinn/core/stage.h"
#include <isl/map.h>
#include <isl/set.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "cinn/core/function.h"
#include "cinn/core/isl_code_gen.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/name_generator.h"
#include "cinn/utils/string.h"

namespace cinn {
using interval_tuple_t = ir::Reference::interval_tuple_t;

void Stage::ExtractDomainFromExpr(Expr x) {
  LOG_INDENT("Stage::ExtractDomainFromExpr");
  CINN_DEBUG(3) << "expr.type: " << ir::GetNodeTyRepr(x.type());
  CINN_DEBUG(3) << "expr: " << ir::Dump(x);
  class Collector : public ir::IRVisitor {
    std::vector<ir::Var> iterators_;
    bool in_reference_{false};

   public:
    Collector() : ir::IRVisitor() {}

    const std::vector<ir::Var>& iterators() { return iterators_; }

    void Visit(const Expr* op) override { IRVisitor::Visit(op); }
    void Visit(const ir::Var* var) override {
      LOG_INDENT("Collector::Visit Var");
      if (!in_reference_) return;
      // check if exists
      auto it = std::find_if(iterators_.begin(), iterators_.end(), [&](const ir::Var& o) { return o == *var; });
      if (it == iterators_.end()) {
        iterators_.push_back(*var);
      }
    }
    void Visit(const ir::Reference* op) override {
      LOG_INDENT("Collector::Visit Reference");
      in_reference_ = true;
      for (auto& x : op->iterators) {
        Visit(&x);
      }
      in_reference_ = false;
    }
    void Visit(const Function* op) override {
      LOG_INDENT("Collector::Visit Function");
      for (auto& x : op->inputs()) {
        Visit(&x);
      }
      for (auto& x : op->outputs()) {
        Visit(&x);
      }
      for (auto& stage : op->stages()) {
        Visit(&stage.expr());
      }
    }
  };

  Collector collector;
  collector.Visit(&x);
  const auto& iterators = collector.iterators();
  CINN_DEBUG(3) << "collect " << iterators.size() << " iterators";

  if (iterators.empty()) return;
  // construct the set
  std::vector<std::string> iterator_names;
  std::transform(iterators.begin(), iterators.end(), std::back_inserter(iterator_names), [](const ir::Var& x) {
    return x.name();
  });
  std::string statement = name() + "[" + Concat(iterator_names, ", ") + "]";

  std::vector<std::string> sub_domains;
  std::transform(iterators.begin(), iterators.end(), std::back_inserter(sub_domains), [](const ir::Var& x) {
    std::stringstream ss;
    ss << x.interval().lower_bound().As<int32_t>() << " <= " << x.name() << " < "
       << x.interval().upper_bound().As<int32_t>();
    return ss.str();
  });

  std::stringstream repr;
  repr << "{ " << statement << ": " << Concat(sub_domains, " and ") << " }";
  std::string format = repr.str();
  CINN_DEBUG(2) << "constructs iterator domain repr: " << format;
  data_->iter_domain = isl::manage(isl_set_read_from_str(ctx(), format.c_str()));
}

Stage::Stage(Expr expr) {
  LOG_INDENT("Stage::Stage");
  InitData();
  data_->expr = expr;
  set_name(NameGenerator::Global().NewStageName());
  CINN_DEBUG(2) << "stage set name " << name();

  ExtractDomainFromExpr(expr);

  if (expr.is_assign()) {
    InitFromAssignExpr(expr);
    InitSchedule();
    InitReadDependencies();
    InitWriteDependencies();
  } else if (expr.is_allocate()) {
    InitFromAllocateExpr(expr);
  }

  Generator::Global().RegisterStage(data_->name, this);
}

void Stage::InitFromAssignExpr(Expr expr) {}

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
  CINN_DEBUG(4) << "schedule: " << schedule;

  schedule = isl::manage(isl_map_coalesce(schedule.release()));
  data_->schedule = schedule;

  CINN_DEBUG(4) << "after init: " << data_->schedule;
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

void Stage::set_name(const std::string& name) {
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
  data_ = std::make_shared<Data>();
  data_->ctx = Generator::Global().ctx().get();
}

isl::map Stage::GetTransformedSchedule() {
  CHECK(!data_->iter_domain.is_null());
  CHECK(!data_->schedule.is_null());
  return data_->schedule.intersect_domain(data_->iter_domain);
}

void Stage::InitFromAllocateExpr(Expr x) {}

bool Stage::is_assign() const { return expr().is_assign(); }

bool Stage::is_allocate() const { return expr().is_allocate(); }

std::set<std::string> Stage::Data::names;

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

namespace {

class ReferenceCollector : public ir::IRPrinter {
  std::set<std::string>& statements;
  std::stringstream& ss_;

 public:
  ReferenceCollector(std::stringstream& os, std::set<std::string>& statements)
      : ss_(os), ir::IRPrinter(os), statements(statements) {
    set_reference_braces("[]");
  }

  void Visit(const ir::Reference* op) override {
    ss_.str("");
    IRPrinter::Visit(op);
    statements.insert(ss_.str());
    ss_.str("");
  }

  void Visit(const Expr* op) override { IRPrinter::Visit(op); }
};

}  // namespace

isl::union_map CollectAccess(const isl::set& iterator_domain, const Expr& expr) {
  // init read access
  std::set<std::string> stmts;
  std::stringstream ss;
  ReferenceCollector collector(ss, stmts);
  collector.Visit(&expr);

  if (stmts.empty()) {
    CINN_DEBUG(2) << "no access found";
    auto space = iterator_domain.space();
    isl::union_map result = isl::manage(isl_union_map_from_map(isl_map_empty(iterator_domain.space().release())));
    return result;
  }
  // collect the Assign RHS
  CINN_DEBUG(2) << "collected " << stmts.size() << " accesses ";
  CINN_DEBUG(4) << "repr: " << Concat(stmts, ", ");

  std::vector<std::string> reprs;
  auto statement_repr = isl_utils::isl_space_get_statement_repr(iterator_domain.space().get());

  for (auto& stmt : stmts) {
    reprs.push_back(statement_repr + " -> " + stmt);
  }

  ss.str("");
  ss << "{ " << Concat(reprs, "; ") << "}";
  auto final_repr = ss.str();

  isl_ctx* ctx = isl_set_get_ctx(iterator_domain.get());
  isl::union_map result(ctx, final_repr.c_str());
  return result;
}

void Stage::InitReadDependencies() {
  if (iterator_domain().is_null()) return;
  CHECK(expr().is_assign());
  LOG_INDENT("Stage::InitReadDependencies");
  CHECK(!iterator_domain().is_null());
  CHECK(!read_access()) << "duplicate init read_access";
  set_read_access(isl::manage(isl_union_map_empty(isl_set_get_space(iterator_domain().get()))));

  auto* assign_expr = expr().As<ir::Assign>();
  set_read_access(CollectAccess(iterator_domain(), assign_expr->b));
  CINN_DEBUG(2) << "get read dependency: " << isl_union_map_to_str(read_access());
}

void Stage::InitWriteDependencies() {
  if (iterator_domain().is_null()) return;
  CHECK(expr().is_assign());
  LOG_INDENT("Stage::InitWriteDependencies");
  CHECK(!iterator_domain().is_null());
  set_write_access(isl::manage(isl_union_map_empty(isl_set_get_space(iterator_domain().get()))));

  auto* assign_expr = expr().As<ir::Assign>();
  set_write_access(CollectAccess(iterator_domain(), assign_expr->a));
  CINN_DEBUG(2) << "get write dependency: " << isl_union_map_to_str(write_access());
}

std::ostream& operator<<(std::ostream& os, Stage::Type t) {
  switch (t) {
    case Stage::Type::polyhedral:
      os << "polyhedral";
      break;
    case Stage::Type::function_call:
      os << "function_call";
      break;
    default:
      os << "unk";
      break;
  }
  return os;
}

}  // namespace cinn
