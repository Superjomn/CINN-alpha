#include "cinn/core/code_gen.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"

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
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_for);
  LOG(INFO) << "get isl ast for node";

  // iter name
  isl_ast_expr* iter = isl_ast_node_for_get_iterator(node);
  isl_id* iter_id = isl_ast_expr_get_id(iter);
  std::string iter_name = isl_id_get_name(iter_id);
  VLOG(3) << "For iter: " << iter_name;

  // get condition
  isl_ast_expr* condition = isl_ast_node_for_get_cond(node);
  isl_ast_expr* incrementor = isl_ast_node_for_get_inc(node);
  isl_ast_expr* initializer = isl_ast_node_for_get_init(node);
  isl_ast_node* body = isl_ast_node_for_get_body(node);

  ir::Expr ir_body;
  IslAstNodeToCinnExpr(body, &ir_body);
  ir_body = ir::Block::make({ir_body});
  VLOG(3) << "for get body " << ir::Dump(ir_body);

  ir::Expr ir_initializer;
  IslAstExprToCinnExpr(initializer, &ir_initializer);
  VLOG(3) << "for get initializer " << ir::Dump(ir_initializer);

  ir::Expr ir_condition;
  IslAstExprToCinnExpr(condition, &ir_condition);
  ir::Expr tmp;
  IslAstExprToCinnExpr(isl_ast_expr_get_op_arg(condition, 1), &tmp);
  VLOG(3) << "for get condition " << ir::Dump(ir_condition);

  ir::Expr ir_inc;
  IslAstExprToCinnExpr(incrementor, &ir_inc);
  VLOG(3) << "for get inc " << ir::Dump(ir_inc);

  ir::Var ir_iter(iter_name);
  VLOG(3) << "for get iter  " << ir::Dump(ir_iter);

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

isl_ast_expr* CreateIslAstIndexExpression(isl_ast_build* build, const isl::map& access) {
  CHECK(build);
  isl::map schedule = isl::manage(isl_map_from_union_map(isl_ast_build_get_schedule(build)));
  LOG(INFO) << "schedule: " << schedule;

  isl::map map = isl::manage(isl_map_reverse(schedule.copy()));
  LOG(INFO) << "schedule reversed: " << map;

  isl::pw_multi_aff iterator_map = isl::manage(isl_pw_multi_aff_from_map(map.copy()));
  LOG(INFO) << "iterator_map: " << iterator_map;

  isl::pw_multi_aff index_aff = isl::manage(isl_pw_multi_aff_from_map(access.copy()));
  LOG(INFO) << "index_aff: " << index_aff;

  isl::space model2 = iterator_map.space();
  index_aff = isl::manage(isl_pw_multi_aff_align_params(index_aff.copy(), model2.copy()));
  isl::space model = index_aff.space();
  LOG(INFO) << "model: " << model;
  iterator_map = isl::manage(isl_pw_multi_aff_align_params(iterator_map.copy(), model.copy()));
  LOG(INFO) << "iterator_map: " << iterator_map;
  iterator_map = isl::manage(isl_pw_multi_aff_pullback_pw_multi_aff(index_aff.copy(), iterator_map.copy()));
  LOG(INFO) << "iterator_map: " << iterator_map;

  isl_ast_expr* index_expr = isl_ast_build_access_from_pw_multi_aff(build, iterator_map.copy());
  return index_expr;
}

std::map<std::string, isl_ast_expr*> ExtractIslTransformedIndiceMap(const isl::set& iterator_domain,
                                                                    isl_ast_build* build) {
  std::map<std::string, isl_ast_expr*> iterator_map;
  isl::map identity = isl::manage(isl_set_identity(iterator_domain.copy()));
  isl::map schedule = identity;

  LOG(INFO) << "schedule: " << schedule;

  identity = identity.apply_domain(schedule);
  LOG(INFO) << "identity: " << identity;

  isl_ast_expr* idx_expr = CreateIslAstIndexExpression(build, identity);

  isl::space domain_space = iterator_domain.space();

  for (int i = 1; i < isl_ast_expr_get_op_n_arg(idx_expr); i++) {
    if (isl_space_has_dim_name(domain_space.get(), isl_dim_set, i - 1)) {
      std::string original_idx_name = isl_space_get_dim_name(domain_space.get(), isl_dim_set, i - 1);
      isl_ast_expr* transformed_index = isl_ast_expr_get_op_arg(idx_expr, i);
      iterator_map.emplace(original_idx_name, transformed_index);
      LOG(INFO) << "idx: " << original_idx_name << " " << isl_ast_expr_to_C_str(transformed_index);
    }
  }
  return iterator_map;
}

}  // namespace cinn
