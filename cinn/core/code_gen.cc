#include "cinn/core/code_gen.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/macros.h"

namespace cinn {

// Eat an isl block node.
void EatBlock(isl_ast_node* node, ir::Expr* expr);
// Eat an isl user node.
void EatUser(isl_ast_node* node, ir::Expr* expr);
// Eat an isl for node.
void EatFor(isl_ast_node* node, ir::Expr* expr);
// Eat an isl `if` node.
void EatIf(isl_ast_node* node, ir::Expr* expr);

void IslAstNodeToCinnExpr(isl_ast_node* node, ir::Expr* expr) {
  CHECK(node);
  CHECK(expr);

  switch (isl_ast_node_get_type(node)) {
    case isl_ast_node_block:
      EatBlock(node, expr);
      break;
    case isl_ast_node_for:
      EatFor(node, expr);
      break;
    case isl_ast_node_if:
      EatIf(node, expr);
      break;
    case isl_ast_node_user:
      EatUser(node, expr);
      break;
    default:
      LOG(FATAL) << "Unexpected ISL node type " << isl_ast_node_get_type(node);
      break;
  }
}

// Eat an isl block node.
void EatBlock(isl_ast_node* node, ir::Expr* expr) {
  VLOG(2) << "get isl ast body node";
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_block);
  isl_ast_node_list* list = isl_ast_node_block_get_children(node);
  std::vector<ir::Expr> exprs;
  for (int i = isl_ast_node_list_n_ast_node(list) - 1; i >= 0; i--) {
    isl_ast_node* child = isl_ast_node_list_get_ast_node(list, i);
    // visit child
    ir::Expr child_expr;
    IslAstNodeToCinnExpr(node, &child_expr);
    exprs.push_back(child_expr);
  }
  *expr = ir::Block::make(std::move(exprs));
}
// Eat an isl user node.
void EatUser(isl_ast_node* node, ir::Expr* expr) {
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_user);
  VLOG(2) << "get isl ast user node";
  isl_ast_expr* isl_expr = isl_ast_node_user_get_expr(node);
  IslAstExprToCinnExpr(isl_expr, expr);
}
// Eat an isl `for` node.
void EatFor(isl_ast_node* node, ir::Expr* expr) {
  LOG_INDENT("EatFor");
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_for);
  CINN_DEBUG(3) << "get isl ast for node";

  // iter name
  isl_ast_expr* iter = isl_ast_node_for_get_iterator(node);
  isl_id* iter_id = isl_ast_expr_get_id(iter);
  std::string iter_name = isl_id_get_name(iter_id);
  CINN_DEBUG(3) << "For iter: " << iter_name;

  // get condition
  isl_ast_expr* condition = isl_ast_node_for_get_cond(node);
  isl_ast_expr* incrementor = isl_ast_node_for_get_inc(node);
  isl_ast_expr* initializer = isl_ast_node_for_get_init(node);
  isl_ast_node* body = isl_ast_node_for_get_body(node);

  ir::Expr ir_body;
  IslAstNodeToCinnExpr(body, &ir_body);
  ir_body = ir::Block::make({ir_body});
  CINN_DEBUG(3) << "for get body " << ir::Dump(ir_body);

  ir::Expr ir_initializer;
  IslAstExprToCinnExpr(initializer, &ir_initializer);
  CINN_DEBUG(3) << "for get initializer " << ir::Dump(ir_initializer);

  ir::Expr ir_condition;
  IslAstExprToCinnExpr(condition, &ir_condition);
  ir::Expr tmp;
  IslAstExprToCinnExpr(isl_ast_expr_get_op_arg(condition, 1), &tmp);
  CINN_DEBUG(3) << "for get condition " << ir::Dump(ir_condition);

  ir::Expr ir_inc;
  IslAstExprToCinnExpr(incrementor, &ir_inc);
  CINN_DEBUG(3) << "for get inc " << ir::Dump(ir_inc);

  ir::Var ir_iter(iter_name);
  CINN_DEBUG(3) << "for get iter  " << ir::Dump(ir_iter);

  *expr = ir::For::make(ir_initializer, ir_condition, ir_inc, ir_body, ir_iter);
}

void EatIf(isl_ast_node* node, ir::Expr* expr) {
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_if);
  LOG(INFO) << "get isl ast if node";
  isl_ast_node* then_body = isl_ast_node_if_get_then(node);
  isl_ast_expr* condition = isl_ast_node_if_get_cond(node);

  ir::Expr ir_then_body;
  IslAstNodeToCinnExpr(then_body, &ir_then_body);

  ir::Expr ir_else_body;
  if (isl_ast_node_if_has_else(node)) {
    isl_ast_node* else_body = isl_ast_node_if_get_else(node);
    IslAstNodeToCinnExpr(else_body, &ir_else_body);
  }

  ir::Expr ir_condition;
  IslAstExprToCinnExpr(condition, &ir_condition);

  if (ir_else_body.valid()) {
    *expr = ir::IfThenElse::make(ir_condition, ir_then_body, ir_else_body);
  } else {
    *expr = ir::IfThenElse::make(ir_condition, ir_then_body);
  }
}

void IslAstExprToCinnExpr(isl_ast_expr* node, ir::Expr* expr) {
  switch (isl_ast_expr_get_type(node)) {
    case isl_ast_expr_int: {
      isl_val* val = isl_ast_expr_get_val(node);
      *expr = ir::Expr((int)isl_val_get_num_si(val));
    } break;
    case isl_ast_expr_id: {
      isl_id* id = isl_ast_expr_get_id(node);
      const char* name = isl_id_get_name(id);
      *expr = ir::Var(name);
    } break;
    case isl_ast_expr_op: {
      std::vector<ir::Expr> ops;
      const int n_args = isl_ast_expr_get_op_n_arg(node);

      for (int i = 0; i < n_args; i++) {
        ir::Expr op;
        isl_ast_expr* expr0 = isl_ast_expr_get_op_arg(node, i);
        IslAstExprToCinnExpr(expr0, &op);
        isl_ast_expr_free(expr0);
        ops.push_back(op);
      }
      isl_ast_op_type op_type = isl_ast_expr_get_op_type(node);
      switch (op_type) {
        case isl_ast_op_and:
          *expr = ir::And::make(ops[0], ops[1]);
          break;
        case isl_ast_op_or:
          *expr = ir::Or::make(ops[0], ops[1]);
          break;
        case isl_ast_op_min:
          *expr = ir::Min::make(ops[0], ops[1]);
          break;
        case isl_ast_op_max:
          *expr = ir::Max::make(ops[0], ops[1]);
          break;
        case isl_ast_op_minus:
          *expr = ir::Minus::make(ops[0]);
          break;
        case isl_ast_op_add:
          *expr = ir::Add::make(ops[0], ops[1]);
          break;
        case isl_ast_op_sub:
          *expr = ir::Sub::make(ops[0], ops[1]);
          break;
        case isl_ast_op_mul:
          *expr = ir::Mul::make(ops[0], ops[1]);
          break;
        case isl_ast_op_div:
          *expr = ir::Div::make(ops[0], ops[1]);
          break;
        case isl_ast_op_le:
          *expr = ir::LE::make(ops[0], ops[1]);
          break;
        case isl_ast_op_lt:
          *expr = ir::LT::make(ops[0], ops[1]);
          break;
        case isl_ast_op_ge:
          *expr = ir::GE::make(ops[0], ops[1]);
          break;
        case isl_ast_op_gt:
          *expr = ir::GT::make(ops[0], ops[1]);
          break;
        case isl_ast_op_call: {
          ir::Expr caller_expr = ops.front();
          // TODO make it an string
          CHECK(caller_expr.type() == ir::NodeTy::Var);
          std::string caller = caller_expr.As<ir::Var>()->name();
          ops.erase(ops.begin());
          *expr = ir::Call::make(caller, ops);
        } break;
        default:
          LOG(FATAL) << "unsupported op " << op_type;
      }
    } break;
    default:
      break;
  }
}

void ReplaceUseridWithExpr(ir::Expr root, const std::string& userid, ir::Expr e) {}

isl::ast_expr CreateIslAstIndexExpression(isl_ast_build* build, const isl::map& access) {
  CHECK(build);
  LOG_INDENT("CreateIslAstIndexExpression");
  isl::map schedule = isl::manage(isl_map_from_union_map(isl_ast_build_get_schedule(build)));
  CINN_DEBUG(2) << "schedule: " << schedule;

  isl::map map = isl::manage(isl_map_reverse(schedule.copy()));
  CINN_DEBUG(2) << "schedule reversed: " << map;

  isl::pw_multi_aff iterator_map = isl::manage(isl_pw_multi_aff_from_map(map.copy()));
  CINN_DEBUG(2) << "iterator_map: " << iterator_map;

  isl::pw_multi_aff index_aff = isl::manage(isl_pw_multi_aff_from_map(access.copy()));
  CINN_DEBUG(2) << "index_aff: " << index_aff;

  isl::space model2 = iterator_map.space();
  index_aff = isl::manage(isl_pw_multi_aff_align_params(index_aff.copy(), model2.copy()));
  isl::space model = index_aff.space();
  CINN_DEBUG(2) << "model: " << model;
  iterator_map = isl::manage(isl_pw_multi_aff_align_params(iterator_map.copy(), model.copy()));
  CINN_DEBUG(2) << "iterator_map: " << iterator_map;
  iterator_map = isl::manage(isl_pw_multi_aff_pullback_pw_multi_aff(index_aff.copy(), iterator_map.copy()));
  CINN_DEBUG(2) << "iterator_map: " << iterator_map;

  isl::ast_expr index_expr = isl::manage(isl_ast_build_access_from_pw_multi_aff(build, iterator_map.copy()));
  return index_expr;
}

std::map<std::string, isl::ast_expr> ExtractIslTransformedIndiceMap(const isl::set& iterator_domain,
                                                                    isl_ast_build* build) {
  LOG_INDENT("ExtractIslTransformedIndiceMap");
  std::map<std::string, isl::ast_expr> iterator_map;
  isl::map identity = isl::manage(isl_set_identity(iterator_domain.copy()));
  isl::map schedule = identity;

  CINN_DEBUG(2) << "schedule: " << schedule;

  identity = identity.apply_domain(schedule);
  CINN_DEBUG(2) << "identity: " << identity;

  isl::ast_expr idx_expr = CreateIslAstIndexExpression(build, identity);

  isl::space domain_space = iterator_domain.space();

  for (int i = 1; i < isl_ast_expr_get_op_n_arg(idx_expr.get()); i++) {
    if (isl_space_has_dim_name(domain_space.get(), isl_dim_set, i - 1)) {
      std::string original_idx_name = isl_space_get_dim_name(domain_space.get(), isl_dim_set, i - 1);
      isl::ast_expr transformed_index = isl::manage(isl_ast_expr_get_op_arg(idx_expr.get(), i));
      iterator_map.emplace(original_idx_name, transformed_index);
      CINN_DEBUG(3) << "idx: " << original_idx_name << " " << isl_ast_expr_to_C_str(transformed_index.get());
    }
  }

  CINN_DEBUG(2) << "end extraction";
  return iterator_map;
}

// class Generator

Stage* Generator::GetStageByName(const std::string& name) {
  auto it = stages_.find(name);
  if (it == stages_.end()) return nullptr;
  return it->second;
}

void Generator::RegisterStage(const std::string& name, Stage* x) {
  CHECK(!stages_.count(name)) << "duplicate register a stage called [" << name << "]";
  CHECK(x) << "stage is null";
  stages_[name] = x;
}

std::vector<Stage*> Generator::FilterStagesByDomain(const isl::set& domain) {
  std::vector<Stage*> result;

  for (auto& item : stages_) {
    Stage* stage = item.second;
    const auto& iterator_domain = stage->iterator_domain();

    auto interct = iterator_domain.intersect(domain);
    if (!interct.is_empty()) result.push_back(stage);
  }

  return result;
}

Generator& Generator::Global() {
  static Generator x;
  return x;
}

#define TWO_ARG_OP(op__)                                                \
  case ir::NodeTy::op__: {                                              \
    auto* x = root.As<ir::op__>();                                      \
    CINN_DEBUG(3) << "visit " << #op__;                                 \
    CINN_DEBUG(3) << "a: " << ir::Dump(x->a);                           \
    CINN_DEBUG(3) << "b: " << ir::Dump(x->b);                           \
    ReplaceCinnIndiceWithIslTransformedIndicesHelper(indice_map, x->a); \
    ReplaceCinnIndiceWithIslTransformedIndicesHelper(indice_map, x->b); \
    CINN_DEBUG(3) << "get transformed a: " << ir::Dump(x->a);           \
    CINN_DEBUG(3) << "get transformed b: " << ir::Dump(x->b);           \
    break;                                                              \
  };

void ReplaceCinnIndiceWithIslTransformedIndicesHelper(const std::map<std::string, Expr>& indice_map, Expr& root) {
  LOG_INDENT("ReplaceCinnIndiceWithIslTransformedIndicesHelper");
  CINN_DEBUG(3) << "replacing " << ir::Dump(root);
  switch (root.type()) {
    OP_2_ARGS_FOR_EACH(TWO_ARG_OP);
    case ir::NodeTy::Var: {
      auto* var = root.As<ir::Var>();
      CINN_DEBUG(3) << "var " << var->name() << " " << var->interval().__str__();
      auto it = indice_map.find(var->name());
      if (it != indice_map.end()) {
        root = ir::CopyExpr(it->second);
        break;
      }
      break;
    }
    case ir::NodeTy::Call: {
      CINN_DEBUG(3) << "visit Call " << ir::Dump(root);
      auto* call = root.As<ir::Call>();
      for (auto& it : call->arguments) {
        LOG_INDENT("visit Call");
        CINN_DEBUG(3) << "replacing argument " << ir::Dump(it);
        ReplaceCinnIndiceWithIslTransformedIndicesHelper(indice_map, it);
        CINN_DEBUG(3) << "get " << ir::Dump(it);
      }
      CINN_DEBUG(3) << "get " << ir::Dump(root);
      break;
    }

    case ir::NodeTy::Reference: {
      LOG_INDENT("visit Reference");
      auto* reference = root.As<ir::Reference>();
      std::vector<ir::Var> iterators;
      for (auto& it : reference->iterators) {
        LOG_INDENT("visit Reference.iterators");
        CINN_DEBUG(3) << "replacing " << ir::Dump(it);
        ReplaceCinnIndiceWithIslTransformedIndicesHelper(indice_map, it);
        CINN_DEBUG(3) << "get " << ir::Dump(it);
      }
      CINN_DEBUG(3) << "get " << ir::Dump(root);
      break;
    }
    default:
      LOG(ERROR) << "Unsupported op type: " << static_cast<int>(root.type());
  }
}

Expr ReplaceCinnIndiceWithIslTransformedIndices(const std::map<std::string, isl::ast_expr>& indice_map, Expr& root) {
  // Transform isl expr map to CINN expr map first.
  std::map<std::string, Expr> cinn_expr_indices;
  for (auto& item : indice_map) {
    Expr expr;
    IslAstExprToCinnExpr(item.second.get(), &expr);
    cinn_expr_indices[item.first] = expr;
  }

  // transform all stages.

  // Replace the indices recursively.
  ReplaceCinnIndiceWithIslTransformedIndicesHelper(cinn_expr_indices, root);
  return root;
}

isl_ast_node* IslAstNodeInfoCollect(isl_ast_node* node, isl_ast_build* build, void* user) {
  LOG_INDENT("IslAstNodeInfoCollect");
  // isl::set iterator_domain(isl_ast_node_get_ctx(node), "{S0[i,j]: 0 < i < j < 100 }");
  Stage* stage = Generator::Global().GetComputationByNode(node);
  CHECK(stage);
  auto isl_indice_map = ExtractIslTransformedIndiceMap(stage->iterator_domain(), build);

  std::map<std::string, Expr> cinn_expr_indices;
  CINN_DEBUG(2) << "collected isl_indice_map.size: " << isl_indice_map.size();
  for (auto& item : isl_indice_map) {
    Expr expr;
    IslAstExprToCinnExpr(item.second.get(), &expr);
    cinn_expr_indices[item.first] = expr;
    CINN_DEBUG(2) << "CINN indice expr: " << item.first << " -> " << ir::Dump(expr);
  }

  stage->SetIndiceMap(std::move(cinn_expr_indices));
  return node;
}

#define TWO_PARAM_OP(op__)                  \
  case ir::NodeTy::op__: {                  \
    auto* node = root.As<ir::op__>();       \
    ReplaceExprWithStage(node->a, s, expr); \
    ReplaceExprWithStage(node->b, s, expr); \
    break;                                  \
  }

void ReplaceExprWithStage(Expr& root, const std::string& s, const Expr& expr) {
  LOG_INDENT("ReplaceExprWithStage");
  CINN_DEBUG(3) << "replace";
  CINN_DEBUG(3) << "original code: \n" << ir::Dump(root);
  CINN_DEBUG(3) << "to " << s;
  CINN_DEBUG(3) << "expr:\n" << ir::Dump(expr);

  switch (root.type()) {
    OP_2_ARGS_FOR_EACH(TWO_PARAM_OP);

    case ir::NodeTy::Reference: {
      CINN_DEBUG(3) << "visit Reference";
      auto* node = root.As<ir::Reference>();
      if (node->target.type() == ir::NodeTy::Var) {
        ir::Var* x = node->target.As<ir::Var>();
        CINN_DEBUG(3) << "reference.target.name: " << x->name();
        if (x->name() == s) {
          root = CopyExpr(expr);
          break;
        }
      }
      // recursively process the target and iterator expressions.
      ReplaceExprWithStage(node->target, s, expr);
      for (auto& iterator : node->iterators) {
        ReplaceExprWithStage(iterator, s, expr);
      }
      break;
    }
    case ir::NodeTy::Call: {
      CINN_DEBUG(3) << "visit Call";
      auto* node = root.As<ir::Call>();
      if (node->caller == s) {
        root = CopyExpr(expr);
        break;
      }

      for (auto& arg : node->arguments) {
        ReplaceExprWithStage(arg, s, expr);
      }
      break;
    }

    case ir::NodeTy::For: {
      CINN_DEBUG(3) << "visit For";
      auto* node = root.As<ir::For>();
      ReplaceExprWithStage(node->iter_init, s, expr);
      ReplaceExprWithStage(node->iter_cond, s, expr);
      ReplaceExprWithStage(node->iter_inc, s, expr);
      ReplaceExprWithStage(node->body, s, expr);
      break;
    }

    case ir::NodeTy::Block: {
      CINN_DEBUG(3) << "visit Block";
      auto* node = root.As<ir::Block>();
      for (auto& e : node->list) {
        ReplaceExprWithStage(e, s, expr);
      }
      break;
    }

    default:
      LOG(ERROR) << "not supported type: " << static_cast<int>(root.type());
  }
}

Stage* Generator::GetComputationByNode(isl_ast_node* node) {
  CHECK(node);
  LOG_INDENT("Generator::GetComputationByNode");
  isl_ast_expr* expr = isl_ast_node_user_get_expr(node);
  isl_ast_expr* arg = isl_ast_expr_get_op_arg(expr, 0);
  std::string name = isl_id_get_name(isl_ast_expr_get_id(arg));
  CINN_DEBUG(2) << "get stage name: " << name;
  isl_ast_expr_free(expr);
  isl_ast_expr_free(arg);
  return Generator::Global().GetStageByName(name);
}

}  // namespace cinn
