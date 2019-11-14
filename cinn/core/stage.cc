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

// Compose the iterator domain with the extra condition.
isl::set BuildWithCond(__isl_give isl_set* domain, const std::string& cond) {
  LOG_INDENT(0);
  CINN_DEBUG(3) << "get extra cond " << cond;
  if (cond.empty()) return isl::manage(domain);
  return isl::manage(isl_set_append_cond(domain, cond.c_str()));
}

void Stage::ExtractDomainFromExpr(Expr x) {
  LOG_INDENT(0);
  CINN_DEBUG(1) << "expr.type: " << ir::GetNodeTyRepr(x.type());
  CINN_DEBUG(1) << "expr: " << x;

  if (iterators_in_order.empty()) {
    CINN_DEBUG(3) << "collect " << iterators_in_order.size() << " iterators";
    std::vector<const ir::Var*> iterators = ir::CollectExprNode<ir::Var>(expr());
    std::transform(iterators.begin(), iterators.end(), std::back_inserter(iterators_in_order), [](const ir::Var* x) {
      return *x;
    });
  }
  if (iterators_in_order.empty()) return;
  // construct the set
  std::vector<std::string> iterator_names;
  std::transform(
      iterators_in_order.begin(), iterators_in_order.end(), std::back_inserter(iterator_names), [](const ir::Var& x) {
        return x.name();
      });

  std::string statement = name() + "[" + Concat(iterator_names, ", ") + "]";
  CINN_DEBUG(3) << "get statement: " << statement;

  auto references = ir::CollectExprNode<ir::Reference>(x);
  CHECK(!references.empty());

  // make all the reference's the same space
  for (const auto* ref : references) {
    if (ref->domain.is_null()) continue;  // skip constant iterators.
    auto ref_domain = ref->domain;
    int ref_dim = isl_set_dim(ref->domain.get(), isl_dim_set);
    auto ref_iterators = isl_set_get_dims(ref_domain);

    std::string transform_repr =
        StringFormat("{ [%s] -> [%s] }",
                     Concat(ref_iterators, ", ").c_str(),
                     Join<ir::Var>(iterators_in_order, ", ", [](const ir::Var& x) { return x.name(); }).c_str());
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
  for (int i = 0; i < iterators_in_order.size(); i++) {
    data_->iter_domain = isl::manage(
        isl_set_set_dim_name(data_->iter_domain.release(), isl_dim_set, i, iterators_in_order[i].name().c_str()));
  }

  CINN_DEBUG(3) << "get Stage's domain: " << iterator_domain();
}

Stage::Stage(Expr expr, const std::vector<ir::Var>& iterators) {
  LOG_INDENT(6);
  InitData();
  data_->expr = expr;
  iterators_in_order = iterators;
  set_name(NameGenerator::Global().NewStageName());
  CINN_DEBUG(2) << "stage set name " << name();

  ExtractDomainFromExpr(expr);

  if (expr.is_assign_derived()) {
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

std::string Stage::DumpIslC() const {
  LOG_INDENT(6);
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

  build = isl::manage(isl_ast_build_set_iterators(build.release(), iterators));

  isl::union_map transform = isl::manage(isl_union_map_from_map(t0.copy()));
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule_map(build.get(), transform.release()));
  ss << isl_ast_node_to_C_str(ast.get());
  return ss.str();
}

std::string Stage::DumpAsC() const {
  LOG_INDENT(0);
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
  LOG_INDENT(6);
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
  // CHECK(!data_->names.count(name)) << "duplicate name for Computation, " << name;
  data_->name = name;
  // data_->names.insert(data_->name);
  if (!data_->iter_domain.is_null()) {
    data_->iter_domain = isl::manage(isl_set_set_tuple_name(data_->iter_domain.release(), name.c_str()));
  }

  auto update_access_name = [](isl::union_map& access, const std::string& name) {
    if (!access.is_null()) {
      isl::union_map umap;
      for (int i = 0; i < isl_union_map_n_map(access.get()); i++) {
        isl_map_list* map_list = isl_union_map_get_map_list(access.get());
        isl::map map = isl::manage(isl_map_list_get_at(map_list, i));
        map = isl::manage(isl_map_set_tuple_name(map.release(), isl_dim_in, name.c_str()));
        umap = umap.is_null() ? isl::manage(isl_union_map_from_map(map.release()))
                              : isl::manage(isl_union_map_add_map(umap.release(), map.release()));
      }
    }
  };

  update_access_name(data_->read_access, name);
  update_access_name(data_->write_access, name);

  if (!data_->schedule.is_null()) {
    data_->schedule = isl::manage(isl_map_set_tuple_name(data_->schedule.release(), isl_dim_in, name.c_str()));
    data_->schedule = isl::manage(isl_map_set_tuple_name(data_->schedule.release(), isl_dim_out, name.c_str()));
  }
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

// std::set<std::string> Stage::Data::names;

void Stage::Interchange(ir::Var i, ir::Var j) { Interchange(i.name(), j.name()); }

void Stage::Interchange(const std::string& dim0, const std::string& dim1) {
  data_->transposes.push_back(std::make_pair(dim0, dim1));
}

void Stage::Tile(ir::Var i, size_t w) { data_->tiles[i.name()] = w; }

void Stage::Tile(const std::vector<int>& sizes) { data_->tile_sizes = sizes; }

void Stage::Vectorize(size_t vector_size) {
  CHECK_GT(vector_size, 1);
  CHECK_LT(vector_size, 30);
  data_->vector_width = vector_size;
}

void Stage::Split(const ir::Var& iter, int size) {
  LOG_INDENT(6);
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
  transform = isl::manage(isl_map_set_dim_names(transform.release(), isl_dim_out, target_dims));

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
  ReferenceCollector(std::stringstream& os, std::set<std::string>& statements)  // NOLINT
      : ss_(os),
        ir::IRPrinter(os),
        statements(statements) {
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
  LOG_INDENT(6);
  CINN_DEBUG(6) << "input interator_domain: " << iterator_domain;
  CINN_DEBUG(6) << "input expr: " << expr;
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
  CHECK(expr().is_assign_derived());
  LOG_INDENT(6);
  CHECK(!iterator_domain().is_null());
  CHECK(!read_access()) << "duplicate init read_access";
  set_read_access(isl::manage(isl_union_map_empty(isl_set_get_space(iterator_domain().get()))));

  auto* assign_expr = expr().As<ir::Assign>();
  set_read_access(CollectAccess(iterator_domain(), assign_expr->b));
  CINN_DEBUG(2) << "get read dependency: " << isl_union_map_to_str(read_access());
}

void Stage::InitWriteDependencies() {
  if (iterator_domain().is_null()) return;
  CHECK(expr().is_assign_derived());
  LOG_INDENT(6);
  CHECK(!iterator_domain().is_null());
  set_write_access(isl::manage(isl_union_map_empty(isl_set_get_space(iterator_domain().get()))));

  auto* assign_expr = expr().As<ir::Assign>();
  set_write_access(CollectAccess(iterator_domain(), assign_expr->a));
  CINN_DEBUG(2) << "get write dependency: " << isl_union_map_to_str(write_access());
}

void Stage::SetCond(const ir::Var& iterator, const std::string& cond) {
  data_->iter_domain =
      BuildWithCond(data_->iter_domain.release(), StringFormat("%s %s", iterator.name().c_str(), cond.c_str()));
}

void Stage::SetCond(const ir::Expr& expr, const std::string& cond) {
  data_->iter_domain =
      BuildWithCond(data_->iter_domain.release(), StringFormat("%s %s", ir::Dump(expr).c_str(), cond.c_str()));
}

void Stage::SetCond(const std::string& x) {
  LOG_INDENT(0);
  data_->iter_domain = BuildWithCond(data_->iter_domain.release(), x);
  CINN_DEBUG(2) << "build extra condition: " << data_->iter_domain;
}

Stage::Type Stage::type() const {
  if (expr().is_assign_derived()) return Type::polyhedral;
  switch (expr().type()) {
    case ir::NodeTy::Call:
    case ir::NodeTy::Allocate:
      return Type::function_call;
    default:
      LOG(INFO) << "type: " << expr().type();
      return Type::unk;
  }
}

void Stage::ClearTransforms() {
  data_->tiles.clear();
  data_->tile_sizes.clear();
  data_->transposes.clear();
  data_->stages_fuse_with.clear();
}

void Stage::TileUnroll(const std::vector<int>& sizes) {
  data_->unroll = true;
  data_->tile_sizes = sizes;
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

class IslPrinter : public ir::IRPrinter {
 public:
  explicit IslPrinter(std::ostream& os) : IRPrinter(os) {}

  void Visit(const ir::EQ* op) override {
    Print(op->a);
    os_ << "=";
    Print(op->b);
  }
  void Visit(const ir::Or* op) override {
    Print(op->a);
    os_ << " or ";
    Print(op->b);
  }
  void Visit(const ir::And* op) override {
    Print(op->a);
    os_ << " and ";
    Print(op->b);
  }
};

void Stage::SetCond(ir::Expr expr) {
  std::stringstream os;
  IslPrinter printer(os);
  printer.Print(expr);
  std::string cond = os.str();
  SetCond(cond);
}

}  // namespace cinn
