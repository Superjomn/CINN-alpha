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
  CINN_DEBUG(3) << "get statement: " << statement;

  auto references = ir::CollectExprNode<ir::Reference>(x);

  std::map<std::string, isl::set> iter_domain;
  std::set<std::string> var_names;
  for (auto& ref : references) {
    CHECK(!ref->domain.is_null()) << "reference empty " << ir::Dump(ref->target);
    CINN_DEBUG(3) << "reference domain: " << ref->domain;

    for (int i = 0; i < isl_set_dim(ref->domain.get(), isl_dim_set); i++) {
      var_names.insert(isl_set_get_dim_name(ref->domain.get(), isl_dim_set, i));
    }
  }
  std::vector<std::string> var_names_in_order(var_names.begin(), var_names.end());
  CINN_DEBUG(3) << "variable names collected from all the References: " << Concat(var_names_in_order, ", ");

  // make all the reference's the same space
  for (auto& ref : references) {
    auto ref_domain = ref->domain;
    int set_dim = isl_set_dim(ref->domain.get(), isl_dim_set);
    std::vector<std::string> dim_names;
    for (int i = 0; i < set_dim; i++) {
      dim_names.push_back(isl_set_get_dim_name(ref->domain.get(), isl_dim_set, i));
    }

    std::string transform_repr =
        StringFormat("{ [%s] -> [%s] }", Concat(dim_names, ", ").c_str(), Concat(var_names_in_order, ", ").c_str());
    isl::map transform(isl_utils::global_isl_ctx(), transform_repr.c_str());
    CINN_DEBUG(3) << "transform: " << transform;
    ref_domain = ref_domain.apply(transform);
    *const_cast<isl::set*>(&ref->domain) = ref_domain;
    CINN_DEBUG(3) << "final domain: " << ref->domain;
    // set to Stage's domain
    if (iterator_domain().is_null()) {
      data_->iter_domain = ref->domain;
    } else {
      data_->iter_domain = isl::manage(isl_set_intersect(data_->iter_domain.release(), ref->domain.copy()));
    }
  }

  data_->iter_domain = isl::manage(isl_set_set_tuple_name(data_->iter_domain.release(), name().c_str()));
  // set dim name
  for (int i = 0; i < var_names_in_order.size(); i++) {
    data_->iter_domain =
        isl::manage(isl_set_set_dim_name(data_->iter_domain.release(), isl_dim_set, i, var_names_in_order[i].c_str()));
  }

  CINN_DEBUG(3) << "get Stage's domain: " << iterator_domain();

  return;

  // check if all the Vars has domain, and we will construct the overall domain from each var's domain.
  bool all_var_has_domain = true;
  for (auto& var : collector.iterators()) {
    if (!var.is_domain_valid()) {
      CINN_DEBUG(3) << "var " << var.name() << " not have domain";
      all_var_has_domain = false;
      break;
    }
  }

  CHECK(all_var_has_domain);
  if (all_var_has_domain) {
    isl::union_set domain(isl_utils::global_isl_ctx(), StringFormat("{ %s : }", statement.c_str()));
    for (auto& var : collector.iterators()) {
      LOG(INFO) << "domain: " << var.domain();
    }
  }

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
  CINN_DEBUG(3) << "schedule: " << schedule;
  CINN_DEBUG(3) << "iterator domain: " << iterator_domain();
  // TODO(Superjomn) it's weried here, just rebuild the schedule and fixed space incompatible error
  schedule = isl::map(isl_utils::global_isl_ctx(), GetStreamStr(schedule));
  isl::map t0 = isl::manage(isl_map_intersect_domain(schedule.copy(), data_->iter_domain.copy()));
  isl::set C(data_->ctx, "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));

  // rename iterators
  std::vector<std::string> iterators;
  int transform_range_dims = isl_map_dim(t0.get(), isl_dim_out);
  CINN_DEBUG(2) << "transform: " << t0;
  for (int i = 0; i < transform_range_dims; i++) {
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
  CINN_DEBUG(3) << "schedule: " << schedule;
  CINN_DEBUG(3) << "iterator_domain: " << iterator_domain();
  isl::union_map final = schedule.intersect_domain(data_->iter_domain);
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
  data_->ctx = isl_utils::global_isl_ctx();
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

  CINN_DEBUG(3) << "current schedule: " << schedule();

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

  std::string transform_repr = StringFormat("{ %s[%s] -> %s[%s] }",
                                            name().c_str(),
                                            Concat(from_dims, ", ").c_str(),
                                            name().c_str(),
                                            Concat(to_dims, ", ").c_str());
  isl::map transform(ctx(), transform_repr);

  // set dims
  transform = isl::manage(isl_utils::isl_map_set_dim_names(transform.release(), isl_dim_out, to_dims));
  // transform = isl::manage(isl_utils::isl_map_set_dim_names(transform.release(), isl_dim_in, from_dims));

  CINN_DEBUG(2) << "transform: " << transform;
  CINN_DEBUG(3) << "iterator domain: " << iterator_domain();
  CINN_DEBUG(3) << "schedule: " << data_->schedule;
  data_->schedule = isl::map(ctx(), GetStreamStr(data_->schedule));
  data_->schedule = data_->schedule.apply_range(transform);
}

void Stage::Tile(ir::Var i, size_t iw, ir::Var j, size_t jw) {
  LOG_INDENT("Stage::Tile");
  int posi = isl_map_get_dim_pos_by_name(schedule().get(), isl_dim_out, i.name());
  int posj = isl_map_get_dim_pos_by_name(schedule().get(), isl_dim_out, j.name());

  CHECK_NE(posi, -1) << "No iterator called " << i.name() << " in schedule " << schedule();
  CHECK_NE(posj, -1) << "No iterator called " << j.name() << " in schedule " << schedule();
  CHECK_EQ(posi + 1, posj) << "i and j should be ajacement";

  Split(i, iw);
  Split(j, jw);
  CINN_DEBUG(3) << "final schedule: " << schedule();
}

void Stage::Split(const ir::Var& iter, int size) {
  LOG_INDENT("Stage::Split");
  CHECK(!schedule().is_null());
  CHECK_GT(size, 0);
  CHECK(isl_utils::isl_map_has_dim_name(schedule().get(), isl_dim_out, iter.name()))
      << "iterator " << iter.name() << " not exists in the schedule of stage " << name();

  std::string new_i_name = iter.name() + "_";    // if the i's name is unique, with "_" suffix is still unique.
  std::string new_i_name1 = iter.name() + "__";  // so is the "__" suffix.

  auto out_statement_repr = isl_map_get_statement_repr(schedule().get(), isl_dim_out);
  CINN_DEBUG(3) << "schedule: " << schedule();
  CINN_DEBUG(3) << "out statement of schedule: " << out_statement_repr;

  std::vector<std::string> target_dims;
  std::vector<std::string> target_conds;
  for (int i = 0; i < isl_map_dim(schedule().get(), isl_dim_out); i++) {
    const char* dim_name = isl_map_get_dim_name(schedule().get(), isl_dim_out, i);
    if (iter.name() == dim_name) {
      // split this iterator.
      std::string new_i_repr = StringFormat("%s = floor(%s/%d)", new_i_name.c_str(), iter.name().c_str(), size);
      std::string new_i_repr1 = new_i_name1 + " = " + iter.name() + " % " + std::to_string(size);
      target_conds.push_back(new_i_repr);
      target_conds.push_back(new_i_repr1);
      target_dims.push_back(new_i_name);
      target_dims.push_back(new_i_name1);
    } else {
      target_dims.push_back(dim_name);
    }
  }

  std::string transform_repr = StringFormat("{ %s -> %s[%s]: %s }",
                                            out_statement_repr.c_str(),
                                            name().c_str(),
                                            Concat(target_dims, ", ").c_str(),
                                            Concat(target_conds, " and ").c_str());
  CINN_DEBUG(3) << "transform repr: " << transform_repr;
  isl::map transform(ctx(), transform_repr.c_str());
  transform = isl::manage(isl_utils::isl_map_set_dim_names(transform.release(), isl_dim_out, target_dims));

  CINN_DEBUG(3) << "get transform: " << transform;
  data_->schedule = isl::map(ctx(), GetStreamStr(data_->schedule).c_str());
  data_->schedule = data_->schedule.apply_range(transform);
  CINN_DEBUG(3) << "get final schedule: " << data_->schedule;
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

  void Visit(const ir::Tensor* op) override { ss_ << op->name(); }

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
  LOG_INDENT("CollectAccess");
  CINN_DEBUG(6) << "input interator_domain: " << iterator_domain;
  CINN_DEBUG(6) << "input expr: " << ir::Dump(expr);
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

  CINN_DEBUG(3) << "iterator_domain.space: " << iterator_domain.space();
  std::vector<std::string> reprs;
  auto statement_repr = isl_utils::isl_space_get_statement_repr(iterator_domain.space().get());
  CINN_DEBUG(3) << "statement_repr: " << statement_repr;

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
