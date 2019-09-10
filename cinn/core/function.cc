#include "cinn/core/function.h"
#include "cinn/core/code_gen.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"

namespace cinn {

std::shared_ptr<Function> cinn::Function::make(const std::string& name,
                                               std::vector<Expr> inputs,
                                               std::vector<Expr> outputs,
                                               std::vector<Stage> stages) {
  LOG_INDENT("Function::make");
  auto node = std::make_shared<Function>();
  node->name = name;
  node->argument_exprs = inputs;
  node->stages = stages;
  CHECK(node->ctx_.get());

  node->CollectIteratorDomain();
  node->InitSchedule();
  CINN_DEBUG(2) << "get iterator domain: " << node->iterator_domain_;
  return node;
}

std::vector<std::string> CollectAllIteratorsFromStages(std::vector<Stage>& stages) {
  std::vector<std::string> iters;
  std::set<std::string> iters_set;

  for (auto& stage : stages) {
    const int ndims = isl_set_n_dim(stage.iterator_domain().get());
    for (int i = 0; i < ndims; i++) {
      std::string iter_name = isl_set_get_dim_name(stage.iterator_domain().get(), isl_dim_set, i);
      if (!iters_set.count(iter_name)) {
        iters.push_back(iter_name);
        iters_set.insert(iter_name);
      }
    }
  }
  return iters;
}

void Function::CollectIteratorDomain() {
  LOG_INDENT("Function::CollectIteratorDomain");
  // Collect all the iterators.
  auto iter_names = CollectAllIteratorsFromStages(stages);
  // Combine iterator domain
  CHECK(!iterator_domain_.get());
  iterator_domain_ = isl::manage(isl_union_set_from_set(stages[0].iterator_domain().copy()));
  for (int i = 1; i < stages.size(); i++) {
    iterator_domain_ = isl::manage(isl_union_set_add_set(iterator_domain_.copy(), stages[i].iterator_domain().copy()));
  }

  CINN_DEBUG(2) << "isl union map: " << iterator_domain_;
}

void Function::Accept(ir::IRVisitor* visitor) const {}

std::string Function::DumpIslC() const {
  LOG_INDENT("Function::DumpIslC");
  std::stringstream ss;
  isl::ast_node ast = GenerateIslAst();
  ss << "generated code:\n" << isl_ast_node_to_C_str(ast.get());
  return ss.str();
}

isl::ast_node Function::GenerateIslAst() const {
  LOG_INDENT("Function::DumpIslC");
  CHECK(iterator_domain_.get());
  auto transform = GetFinalTransform();
  isl::set C(ctx_, "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule_map(build.get(), transform.release()));
  isl_ast_build_set_at_each_domain(build.get(), IslAstNodeInfoCollect, nullptr);
  return ast;
}

std::string Function::Dump() const {
  LOG_INDENT("Function::Dump");
  isl::ast_node ast = GenerateIslAst();
  Expr expr;
  IslAstNodeToCinnExpr(ast, &expr);
  return ir::Dump(expr);
}

void Function::InitSchedule() {
  LOG_INDENT("Function::InitSchedule");
  CINN_DEBUG(2) << "stage count: " << stages.size();
  CINN_DEBUG(3) << "union iterator domain: " << iterator_domain();

  for (Stage& stage : stages) {
    isl::set domain = isl::manage(stage.iterator_domain().copy());
    isl_utils::map identity_schedule = isl::manage(isl_set_identity(domain.release()));
    identity_schedule.set_out_tuple_name("");
    CINN_DEBUG(3) << "generate identity schedule for " << stage.name() << " get " << identity_schedule;
    schedule_.emplace_back(identity_schedule);
  }
}

isl::union_map Function::GetFinalTransform() const {
  isl_utils::union_map result = schedule_.front().intersect_domain(stages.front().iterator_domain());
  for (int i = 1; i < stages.size(); i++) {
    result.union_inplace(schedule_[i].intersect_domain(stages[i].iterator_domain()));
  }
  return result;
}

}  // namespace cinn