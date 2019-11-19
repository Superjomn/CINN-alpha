#include "cinn/core/function.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include "cinn/core/isl_code_gen.h"
#include "cinn/core/stage.h"
#include "cinn/core/transform/transforms.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"

namespace cinn {

std::shared_ptr<Function> cinn::Function::make(const std::string& name,
                                               std::vector<Expr> inputs,
                                               std::vector<Expr> outputs,
                                               std::vector<Stage> stages) {
  LOG_INDENT(6);
  auto node = std::make_shared<Function>();
  node->InitData();
  node->data_->name = name;
  node->data_->inputs = inputs;
  node->data_->outputs = outputs;

  for (auto& stage : stages) {
    node->AddStage(stage);
  }
  CHECK(node->data_->ctx);

  node->EndDefinition();

  return node;
}

std::vector<std::string> CollectAllIteratorsFromStages(std::vector<Stage>& stages) {  // NOLINT
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

// TODO(Superjomn) to make the return type from Expr to vector<Expr> to contain multiple expressions and support Call
// and Allocate.
const Expr& Function::ComputeTransformedExpr() const {
  if (data_->transformed_expr.valid()) return data_->transformed_expr;
  data_->transformed_expr = snippets().back().GetTransformedExpr();

  // Only create a new block if there is more than one expressions, to avoid unnecessary block indents.
  if (snippets().size() > 1UL) {
    // Get a block with none or multiple expressions.
    std::vector<Expr> exprs;
    for (auto& snippet : data_->snippets) {
      exprs.push_back(snippet.GetTransformedExpr());
    }
    data_->transformed_expr = ir::Block::make(std::move(exprs));
  }

  return data_->transformed_expr;
}

// This is a naive implementation which has complexity of N^2
void Function::ComputeStageFlows() {
  isl::union_map all_deps;
  for (size_t s1_id = 0; s1_id < data_->stages.size(); s1_id++) {
    for (size_t s0_id = s1_id + 1; s0_id < data_->stages.size(); s0_id++) {
      auto& s0 = data_->stages[s0_id];
      auto& s1 = data_->stages[s1_id];

      CHECK(s0.read_access());
      CHECK(s0.write_access());
      CHECK(s1.read_access());
      CHECK(s1.write_access());

      isl_union_map* deps =
          isl_utils::isl_calculate_dependency(s0.read_access(), s0.write_access(), s1.read_access(), s1.write_access());
      all_deps = all_deps.is_null() ? isl::manage(deps) : isl::manage(isl_union_map_union(all_deps.release(), deps));
    }
  }
}

Stage Function::AddStage(const Stage& stage) {
  data_->stages.push_back(stage);
  return stage;
}

void Function::BuildSnippets(bool end_snippet) {
  LOG_INDENT(6);
  auto& snippets = data_->snippets;
  snippets.clear();

  for (auto& stage : data_->stages) {
    CINN_DEBUG(3) << "add stage: " << stage.name() << " " << stage.expr();
    CINN_DEBUG(4) << "stage.type: " << stage.type();
    CINN_DEBUG(6) << "snippets.size: " << snippets.size();
    // add to snippets
    if (snippets.empty() || snippets.back().is_unk()) {
      snippets.emplace_back();
    } else if (snippets.back().type() != stage.type()) {
      CINN_DEBUG(2) << "snippets.back().type: " << snippets.back().type();
      if (end_snippet) snippets.back().End();
      snippets.emplace_back();
    }
    snippets.back().AddStage(stage);
  }

  if (!snippets.empty() && end_snippet) snippets.back().End();
  CINN_DEBUG(3) << "get snippets size " << snippets.size();
}

Expr Function::operator()(const std::vector<Expr>& inputs, const std::vector<Expr>& outputs) {
  if (!data_->is_inline) {
    std::vector<Expr> args(inputs.begin(), inputs.end());
    std::transform(outputs.begin(), outputs.end(), std::back_inserter(args), [](const Expr& e) { return e; });
    return ir::Call::make(data_->name, args);
  } else {
    // expand
    LOG(FATAL) << "not supported yet";
  }
}

Function::operator Expr() { return data_->ir_function; }

void Snippet::CollectIteratorDomain() {
  LOG_INDENT(6);
  CHECK(Stage::is_polyhedral(type())) << "only polyhedral snippet supports iterator domain";
  iterator_domain_->release();

  // Collect all the iterators.
  auto iter_names = CollectAllIteratorsFromStages(stages_);
  // Combine iterator domain
  CHECK(iterator_domain().is_null());

  for (auto& stage : stages_) {
    if (iterator_domain().is_null()) {
      *iterator_domain_ = isl::manage(isl_union_set_from_set(stage.iterator_domain().copy()));
    } else {
      *iterator_domain_ = isl::manage(isl_union_set_add_set(iterator_domain().copy(), stage.iterator_domain().copy()));
    }
  }

  CINN_DEBUG(3) << "collected iterator domain: " << iterator_domain();
}

void Snippet::AddStage(const Stage& stage) {
  LOG_INDENT(6);
  CHECK(!is_end_) << "definition of the snippet is end, should not add stages.";
  CHECK(stage.type() != Stage::Type::unk);
  CINN_DEBUG(3) << "add a " << stage.type() << " stage called " << stage.name();
  CINN_DEBUG(3) << "snippet type " << type();
  CINN_DEBUG(3) << "stage type " << stage.type();

  if (is_unk()) {
    type_ = stage.type();
  } else {
    CHECK_EQ(type_, stage.type()) << "type not match";
  }
  stages_.push_back(stage);
}

void Snippet::CollectReadAccess() {
  access_reads_->release();
  LOG_INDENT(6);
  CHECK(Stage::is_polyhedral(type()));
  for (auto& stage : stages_) {
    CHECK(stage.read_access());
    if (access_reads_->is_null()) {
      *access_reads_ = isl::manage(isl_union_map_copy(stage.read_access()));
    } else {
      *access_reads_ =
          isl::manage(isl_union_map_union(access_reads_->release(), isl_union_map_copy(stage.read_access())));
    }
  }
  CINN_DEBUG(3) << "collect read access: " << *access_reads_;
}

void Snippet::CollectWriteAccess() {
  access_writes_->release();
  LOG_INDENT(6);
  CHECK(Stage::is_polyhedral(type()));
  for (auto& stage : stages_) {
    CHECK(stage.write_access());
    if (access_writes_->is_null()) {
      *access_writes_ = isl::manage(isl_union_map_copy(stage.write_access()));
    } else {
      *access_writes_ =
          isl::manage(isl_union_map_union(access_writes_->release(), isl_union_map_copy(stage.write_access())));
    }
  }
  CINN_DEBUG(3) << "collect read access: " << *access_writes_;
}

isl::union_map ComputeDeps(const isl::union_set& domain, const isl::union_map& reads, const isl::union_map& writes) {
  isl_ctx* ctx = domain.ctx().get();

  isl::union_map access_reads_with_domain = isl::manage(isl_union_map_intersect_domain(reads.copy(), domain.copy()));
  isl::union_map access_writes_with_domain = isl::manage(isl_union_map_intersect_domain(writes.copy(), domain.copy()));

  isl::union_map reads_writes = access_reads_with_domain;
  reads_writes = isl::manage(isl_union_map_union(reads_writes.release(), access_writes_with_domain.copy()));

  isl::union_map left = isl::manage(
      isl_union_map_apply_range(reads_writes.copy(), isl_union_map_reverse(access_writes_with_domain.copy())));
  CINN_DEBUG(3) << "read_write o write^-1: " << left;
  isl::union_map right = isl::manage(isl_union_map_apply_range(access_writes_with_domain.copy(),
                                                               isl_union_map_reverse(access_reads_with_domain.copy())));
  CINN_DEBUG(3) << "wrie o read^-1: " << right;

  isl::union_map deps = isl::manage(isl_union_map_union(left.release(), right.release()));
  deps = isl::manage(isl_union_map_detect_equalities(deps.release()));
  return deps;
}

//! Compare two stage names by numeric values, that is, "s10" > "s1".
int CompareStageName(const std::string& s0, const std::string& s1) {
  auto s0_ = s0.substr(1);
  auto s1_ = s1.substr(1);
  int s0_val = std::stoi(s0_);
  int s1_val = std::stoi(s1_);
  if (s0_val < s1_val) return -1;
  if (s0_val < s1_val) return 1;
  return 0;
}

isl::union_map ComputeScheduleValidity(const isl::union_set& domain, const isl::union_map& deps) {
  isl::union_map validity = isl::manage(isl_union_map_empty(isl_space_copy(domain.space().get())));
  // currently, we ignore the b->a dependency.
  // TODO(Superjomn) support full analysis for dependencies for any pairs.
  for (int i = 0; i < isl_union_map_n_map(deps.get()); i++) {
    isl_map_list_guard map_list(isl_union_map_get_map_list(deps.get()));
    isl::map map = isl::manage(isl_map_list_get_at(map_list.get(), i));
    if (isl_map_is_identity(map.get())) continue;

    const char* left_tuple = isl_map_get_tuple_name(map.get(), isl_dim_in);
    const char* right_tuple = isl_map_get_tuple_name(map.get(), isl_dim_out);

    if (CompareStageName(left_tuple, right_tuple) >= 0) continue;

    isl::union_map union_map = isl::manage(isl_union_map_from_map(map.copy()));
    if (validity.is_null()) {
      validity = union_map;
    } else {
      validity = isl::manage(isl_union_map_union(validity.release(), union_map.release()));
    }
  }

  return validity;
}

isl::schedule Snippet::ComputeSchedule() {
  // Use a unique ctx to avoid obstruction.
  LOG_INDENT(6);
  CHECK(Stage::is_polyhedral(type()));
  CHECK(!access_reads_->is_null());
  CHECK(!access_writes_->is_null());

  auto domain = isl::union_set(ctx_, GetStreamStr(iterator_domain()));
  auto reads = isl::union_map(ctx_, GetStreamStr(access_reads()));
  auto writes = isl::union_map(ctx_, GetStreamStr(access_writes()));
  auto deps = ComputeDeps(domain, reads, writes);
  auto validity = ComputeScheduleValidity(domain, deps);
  CHECK(!validity.is_null());

  CINN_DEBUG(3) << "get memory dependencies: " << validity;

  BuildFusion();
  isl::union_map proximity;
  if (approxi_) proximity = isl::union_map(ctx_, GetStreamStr(*approxi_));

  isl::schedule_constraints sc = isl::manage(isl_schedule_constraints_on_domain(domain.release()));
  sc = isl::manage(isl_schedule_constraints_set_validity(sc.release(), validity.release()));
  if (!proximity.is_null()) sc = isl::manage(isl_schedule_constraints_set_proximity(sc.release(), proximity.release()));

  CINN_DEBUG(3) << "schedule constraints:\n" << sc;

  *schedule_ = isl::manage(isl_schedule_constraints_compute_schedule(sc.release()));
  CINN_DEBUG(3) << "schedule:\n" << DumpSchedule(*schedule_);

  return *schedule_;
}

void Snippet::ApplyTransforms() {
  ApplyTransposes();
  ApplyTiles();
  ApplyVectorize();
}

void Snippet::ApplyTiles() {
  if (!is_polyhedral()) return;
  CHECK(schedule_) << "schedule tree should be build first before tile";
  LOG_INDENT(0);

  for (auto& stage : stages_) {
    {
      CINN_DEBUG(2) << stage.name() << " "
                    << " tile_sizes " << stage.tile_sizes().size();
      if (!stage.tile_sizes().empty()) {
        if (stage.unroll()) {
          TileUnrollTransformer tiler(stage.name(), stage.tile_sizes());
          *schedule_ = tiler.Visit(*schedule_).get_schedule();
        } else {
          TileDimsTransformer tiler(stage.name(), stage.tile_sizes());
          *schedule_ = tiler.Visit(*schedule_).get_schedule();
        }
      }
    }
    {
      for (auto& item : stage.tiles()) {
        CHECK_GE(item.second, 2);
        CHECK_LT(item.second, 512);
        TileSingleDimTransformer tiler(stage.name(), item.first, item.second);
        *schedule_ = tiler.Visit(*schedule_).get_schedule();
      }
    }
  }
}

void Snippet::ApplyTransposes() {
  if (!is_polyhedral()) return;
  CHECK(schedule_) << "schedule tree should be built first";
  LOG_INDENT(0);

  for (auto& stage : stages_) {
    for (auto& item : stage.transposes()) {
      CINN_DEBUG(2) << "Transposing " << stage.name() << " with " << item.first << " " << item.second;
      InterchangeTransformer applyer(stage.name(), item.first, item.second);
      *schedule_ = applyer.Visit(*schedule_).get_schedule();
    }
  }
}

void Snippet::ApplyVectorize() {
  if (!is_polyhedral()) return;
  CHECK(schedule_) << "schedule tree should be built first";

  for (auto& stage : stages_) {
    if (stage.vector_width() > 0) {
      VectorizeTransform applyer(stage.name(), stage.vector_width());
      *schedule_ = applyer(*schedule_).get_schedule();
    }
  }
}

void Snippet::BuildFusion() {
  for (auto& stage : stages_) {
    std::string this_stage = stage.name();
    for (auto& target : stage.stages_fuse_with()) {
      auto it = std::find_if(stages_.begin(), stages_.end(), [&](const Stage& o) { return o.name() == target; });
      CHECK(it != stages_.end());

      auto this_statement = isl_set_get_statement_repr(stage.iterator_domain().get());
      auto target_statement = isl_set_get_statement_repr(it->iterator_domain().get());

      isl::union_map map(isl_utils::global_isl_ctx(),
                         StringFormat("{ %s -> %s }", this_statement.c_str(), target_statement.c_str()).c_str());
      if (!approxi_) {
        approxi_.reset(new isl::union_map);
        *approxi_ = isl::manage(map.release());
      } else {
        *approxi_ = isl::manage(isl_union_map_union(approxi_->release(), map.release()));
      }
    }
  }
}

isl::ast_node Snippet::GenerateIslAst() const {
  LOG_INDENT(2);
  isl::ast_node res;
  if (!is_polyhedral()) return res;

  CHECK(!iterator_domain().is_null());

  // TODO(Superjomn) pass the parameters.
  isl::set C(isl_utils::global_isl_ctx(), "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));

  isl_options_set_tile_scale_tile_loops(isl_utils::global_isl_ctx(), 0);
  isl_options_set_tile_shift_point_loops(isl_utils::global_isl_ctx(), 0);

  build = isl::manage(isl_ast_build_set_at_each_domain(build.release(), IslAstNodeInfoCollect, nullptr));
  CINN_DEBUG(0) << "schedule in Snippet::GenerateIslAst: \n" << schedule_->root();
  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule(build.get(), schedule_->copy()));
  CINN_DEBUG(0) << "schedule tree get C code:\n" << isl_ast_node_to_C_str(ast.get());
  return ast;
}

Expr Snippet::GetTransformedExpr() const {
  LOG_INDENT(6);
  CHECK(is_end_);

  if (!is_polyhedral()) {
    if (stages_.size() == 1) {
      return stages_.back().expr();
    } else {
      // collect none or multiple stages.
      std::vector<Expr> exprs;
      for (auto& stage : stages_) {
        // TODO(Superjomn) need CopyExpr here ?
        CINN_DEBUG(3) << "collect non-polyhedral expr " << stage.expr();
        exprs.emplace_back(stage.expr());
      }
      return ir::Block::make(std::move(exprs));
    }
  }

  // a polyhedral snippet
  isl::ast_node ast = GenerateIslAst();
  Expr expr;
  IslAstNodeToCinnExpr(ast, &expr);
  for (int i = 0; i < stages_.size(); i++) {
    AttachCinnExprToIslIndices(expr, stages_[i].name());
  }
  return expr;
}

void Snippet::TryFuse(const std::string& stage0, const std::string& stage1) {
  // colect a map from name to stage pointer.
  std::map<std::string, Stage*> map;
  for (auto& stage : stages_) {
    map[stage.name()] = &stage;
  }

  // will try to fuse if these two stage exists in the same snippet.
  if (map.count(stage0) && map.count(stage1)) {
    Stage& a = *map[stage0];
    Stage& b = *map[stage1];
    a.iterator_domain();
  }
}

void Snippet::End() {
  is_end_ = true;

  if (is_polyhedral()) {
    CollectIteratorDomain();
    CollectReadAccess();
    CollectWriteAccess();
    ComputeSchedule();
    ApplyTransforms();
  }
}

namespace {

class BandCollectFirstStatement : public ScheduleNodeRewriter<BandCollectFirstStatement> {
 public:
  using BaseTy = ScheduleNodeRewriter<BandCollectFirstStatement>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  explicit BandCollectFirstStatement(std::set<std::string>* statements) : statements_(*statements) {}

  isl::schedule_node VisitBand(const isl::schedule_node& node) {
    auto domain = isl::manage(isl_schedule_node_get_domain(node.get()));
    LOG(INFO) << "visit domain " << domain;
    isl::set first_set = isl::manage(isl_union_set_get_nth_element(domain.get(), 0));
    statements_.insert(isl_set_get_tuple_name(first_set.get()));

    return Visit(node.first_child()).parent();
  }

 private:
  std::set<std::string>& statements_;
};

}  // namespace

std::set<std::string> Snippet::CollectBandFirstStatement() {
  std::set<std::string> statements;
  if (!is_polyhedral()) return statements;

  CollectIteratorDomain();
  CollectReadAccess();
  CollectWriteAccess();
  ComputeSchedule();

  LOG(INFO) << "collect band first statement schedule:\n" << schedule_->root();

  BandCollectFirstStatement collector(&statements);
  collector.Visit(*schedule_);

  return statements;
}

}  // namespace cinn
