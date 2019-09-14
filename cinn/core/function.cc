#include "cinn/core/function.h"
#include "cinn/core/isl_code_gen.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"
#include "function.h"

namespace cinn {

std::shared_ptr<Function> cinn::Function::make(const std::string& name,
                                               std::vector<Expr> inputs,
                                               std::vector<Expr> outputs,
                                               std::vector<Stage> stages) {
  LOG_INDENT("Function::make");
  auto node = std::make_shared<Function>();
  node->name = name;
  node->inputs = inputs;
  node->outputs = outputs;
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

  for (auto& stage : stages) {
    if (stage.is_assign()) {
      if (iterator_domain_.is_null()) {
        iterator_domain_ = isl::manage(isl_union_set_from_set(stage.iterator_domain().copy()));
      } else {
        iterator_domain_ = isl::manage(isl_union_set_add_set(iterator_domain_.copy(), stage.iterator_domain().copy()));
      }
    }
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
  LOG_INDENT("Function::GenerateIslAst");
  CHECK(iterator_domain_.get());
  auto transform = GetFinalTransform();
  CINN_DEBUG(2) << "final transformed schedule: " << transform;
  isl::set C(ctx_, "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));
  build = isl::manage(isl_ast_build_set_at_each_domain(build.release(), IslAstNodeInfoCollect, nullptr));
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule_map(build.get(), transform.release()));
  return ast;
}

std::string Function::Dump() const {
  LOG_INDENT("Function::Dump");
  isl::ast_node ast = GenerateIslAst();
  Expr expr;
  IslAstNodeToCinnExpr(ast, &expr);
  for (int i = 0; i < stages.size(); i++) {
    if (stages[i].is_assign()) {
      ReplaceExprWithStage(expr, stages[i].name(), stages[i].GetIndiceTransformedExpr());
    }
  }
  return ir::Dump(expr);
}

void Function::InitSchedule() {
  LOG_INDENT("Function::InitSchedule");
  CINN_DEBUG(2) << "stage count: " << stages.size();
  CINN_DEBUG(3) << "union iterator domain: " << iterator_domain();

  for (Stage& stage : stages) {
    isl_utils::map identity_schedule;
    if (stage.is_assign()) {
      isl::set domain = isl::manage(stage.iterator_domain().copy());
      identity_schedule = isl::manage(isl_set_identity(domain.release()));
      identity_schedule.set_out_tuple_name("");
      CINN_DEBUG(3) << "generate identity schedule for " << stage.name() << " get " << identity_schedule;
      schedule_.emplace(stage.name(), identity_schedule);
    }
  }
}

isl::union_map Function::GetFinalTransform() const {
  std::vector<isl_utils::map> schedules;
  for (auto& x : schedule_) {
    schedules.emplace_back(x.second);
  }

  isl_utils::union_map result;
  for (auto& stage : stages) {
    if (schedule_.count(stage.name())) {
      auto& schedule = schedule_.at(stage.name());
      if (result.is_null()) {
        result = schedule.intersect_domain(stage.iterator_domain());
      } else {
        result.union_inplace(schedule.intersect_domain(stage.iterator_domain()));
      }
    }
  }
  return result;
}

Expr Function::GetTransformedExpr() const {
  LOG_INDENT("Function::GetTransformedExpr");
  isl::ast_node ast = GenerateIslAst();
  Expr expr;
  IslAstNodeToCinnExpr(ast, &expr);
  for (int i = 0; i < stages.size(); i++) {
    if (!stages[i].is_assign()) continue;
    ReplaceExprWithStage(expr, stages[i].name(), stages[i].GetIndiceTransformedExpr());
  }
  return expr;
}

void Function::PreAppendStage(const Stage& stage) {
  LOG_INDENT("Function::PreAppendStage");
  stages.insert(stages.begin(), stage);
}

Function::Function() : ctx_(Generator::Global().ctx().get()) {}

}  // namespace cinn