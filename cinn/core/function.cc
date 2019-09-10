#include "cinn/core/function.h"
#include "cinn/core/code_gen.h"
#include "cinn/core/stage.h"
#include "cinn/utils/logging.h"

namespace cinn {

Expr cinn::Function::make(const std::string& name,
                          std::vector<Expr> inputs,
                          std::vector<Expr> outputs,
                          std::vector<Stage> stages) {
  LOG_INDENT("Function::make");
  auto node = std::make_shared<Function>();
  node->name = name;
  node->argument_exprs = inputs;
  node->body = stages;
  CHECK(node->ctx_.get());

  node->CollectIteratorDomain();
  CINN_DEBUG(2) << "get iterator domain: " << node->iterator_domain_;
  auto expr = Expr(node);
  return expr;
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
  auto iter_names = CollectAllIteratorsFromStages(body);
  // Combine iterator domain
  CHECK(!iterator_domain_.get());
  iterator_domain_ = isl::manage(isl_union_set_from_set(body[0].iterator_domain().copy()));
  for (int i = 1; i < body.size(); i++) {
    iterator_domain_ = isl::manage(isl_union_set_add_set(iterator_domain_.copy(), body[i].iterator_domain().copy()));
  }

  CINN_DEBUG(2) << "isl union map: " << iterator_domain_;
}

void Function::Accept(ir::IRVisitor* visitor) const {}

std::string Function::DumpIslC() const {
  LOG_INDENT("Function::DumpIslC");
  std::stringstream ss;
  CHECK(iterator_domain_.get());
  isl::map schedule;
  // schedule = schedule_ ? schedule_ : isl_set_to_identity_map(iter_domain_);
  CINN_DEBUG(2) << "iter domain " << iterator_domain_;
  // auto* iter_domain = isl_union_set_read_from_str(ctx_, "{ S0[i,j]: 0 <= i < 100 and 0 <= j < 150; S1[i,j]: 0 <= i <
  // 100 and 0 <= j < 150} ");
  auto iter_domain = iterator_domain_;
  CINN_DEBUG(2) << "iter domain " << iter_domain;
  isl::union_map map(ctx_, "{ S0[i,j] -> [0, i,j]; S1[i,j] -> [1, i,j] }");
  CINN_DEBUG(2) << "map " << map;
  auto applied = isl::manage(isl_union_map_intersect_domain(map.copy(), iter_domain.copy()));
  CINN_DEBUG(2) << "applied " << applied;
  // auto* C = isl_set_empty(isl_space_copy(isl_union_map_get_space(T)));
  isl::set C(ctx_, "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule_map(build.get(), applied.release()));
  ss << "generated code:\n" << isl_ast_node_to_C_str(ast.get());
  return ss.str();
}

void Function::GenerateIslAst() {
  LOG_INDENT("Function::GenerateIslAst");
  CHECK(iterator_domain_.get());
  isl::set context(ctx_, "{:}");

  CINN_DEBUG(2) << "get iterator_domain " << iterator_domain_;
  auto* build = isl_ast_build_from_context(context.copy());
  isl_ast_build_set_at_each_domain(build, IslAstNodeInfoCollect, nullptr);

  isl::union_map transform(iterator_domain().ctx(), "{ [i,j] -> [i+1, j] }");
  isl::union_map schedule(iterator_domain().ctx(), "{ S0[i,j] -> [i, 0, j, 0]; S1[i,j] -> [i, 0, j, 1] }");
  CHECK(schedule.get());
  schedule = schedule.intersect_domain(iterator_domain_);
  CINN_DEBUG(2) << "schedule: " << schedule;

  auto* ast = isl_ast_build_node_from_schedule_map(build, schedule.copy());
  isl_ast_build_free(build);

  // Expr expr;
  // IslAstNodeToCinnExpr(ast, &expr);
}

std::string Function::Dump() const {}

}  // namespace cinn