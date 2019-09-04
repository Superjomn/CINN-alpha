
#include "cinn/core/function.h"
#include "cinn/core/stage.h"

namespace cinn {

Expr cinn::Function::make(const std::string& name,
                          std::vector<Expr> inputs,
                          std::vector<Expr> outputs,
                          std::vector<Stage> stages) {
  auto node = std::make_shared<Function>();
  node->name = name;
  node->argument_exprs = inputs;
  node->body = stages;
  node->ctx_ = stages.front().ctx();

  node->ScheduleByStageOrder();
  return Expr(node);
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

void Function::ScheduleByStageOrder() {
  // Collect all the iterators.
  auto iter_names = CollectAllIteratorsFromStages(body);
  // Combine iterator domain
  CHECK(!iterator_domain_.get());
  iterator_domain_ = isl::manage(isl_union_set_from_set(body[0].iterator_domain().copy()));
  for (int i = 1; i < body.size(); i++) {
    iterator_domain_ = isl::manage(isl_union_set_add_set(iterator_domain_.copy(), body[i].iterator_domain().copy()));
  }

  LOG(INFO) << "isl union map: " << iterator_domain_;
}

void Function::Accept(ir::IRVisitor* visitor) const {}

std::string Function::DumpIslC() const {
  std::stringstream ss;
  CHECK(ctx_);
  CHECK(iterator_domain_.get());
  isl::map schedule;
  // schedule = schedule_ ? schedule_ : isl_set_to_identity_map(iter_domain_);
  LOG(INFO) << "iter domain " << iterator_domain_;
  // auto* iter_domain = isl_union_set_read_from_str(ctx_, "{ S0[i,j]: 0 <= i < 100 and 0 <= j < 150; S1[i,j]: 0 <= i <
  // 100 and 0 <= j < 150} ");
  auto iter_domain = iterator_domain_;
  LOG(INFO) << "iter domain " << iter_domain;
  isl::union_map map(ctx_, "{ S0[i,j] -> [0, i,j]; S1[i,j] -> [0, i,j] }");
  LOG(INFO) << "map " << map;
  auto applied = isl::manage(isl_union_map_intersect_domain(map.copy(), iter_domain.copy()));
  LOG(INFO) << "applied " << applied;
  // auto* C = isl_set_empty(isl_space_copy(isl_union_map_get_space(T)));
  isl::set C(ctx_, "{:}");
  auto* build = isl_ast_build_from_context(C.copy());
  auto* ast = isl_ast_build_node_from_schedule_map(build, applied.release());
  ss << "generated code:\n" << isl_ast_node_to_C_str(ast);
  return ss.str();
}

void Function::GenerateIslAst() {}

std::string Function::Dump() const {}

}  // namespace cinn