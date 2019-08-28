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

void CinnExprFromIslAstExpr(isl_ast_expr* node, ir::Expr* expr);

void WalkIslAst(isl_ast_node* node, ir::Expr* expr) {
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
    WalkIslAst(node, &child_expr);
    exprs.push_back(child_expr);
  }
  *expr = ir::Block::make(std::move(exprs));
}
// Eat an isl user node.
void EatUser(isl_ast_node* node, ir::Expr* expr) {
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_user);
  VLOG(2) << "get isl ast user node";
  isl_ast_expr* isl_expr = isl_ast_node_user_get_expr(node);
  CinnExprFromIslAstExpr(isl_expr, expr);
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
  WalkIslAst(body, &ir_body);
  ir_body = ir::Block::make({ir_body});
  VLOG(3) << "for get body " << ir::Dump(ir_body);

  ir::Expr ir_initializer;
  CinnExprFromIslAstExpr(initializer, &ir_initializer);
  VLOG(3) << "for get initializer " << ir::Dump(ir_initializer);

  ir::Expr ir_condition;
  CinnExprFromIslAstExpr(condition, &ir_condition);
  ir::Expr tmp;
  CinnExprFromIslAstExpr(isl_ast_expr_get_op_arg(condition, 1), &tmp);
  LOG(INFO) << "*** condition " << Dump(tmp);
  VLOG(3) << "for get condition " << ir::Dump(ir_condition);

  ir::Expr ir_inc;
  CinnExprFromIslAstExpr(incrementor, &ir_inc);
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
  WalkIslAst(then_body, &ir_then_body);

  ir::Expr ir_else_body;
  if (isl_ast_node_if_has_else(node)) {
    isl_ast_node* else_body = isl_ast_node_if_get_else(node);
    WalkIslAst(else_body, &ir_else_body);
  }

  ir::Expr ir_condition;
  CinnExprFromIslAstExpr(condition, &ir_condition);

  if (ir_else_body.valid()) {
    *expr = ir::IfThenElse::make(ir_condition, ir_then_body, ir_else_body);
  } else {
    *expr = ir::IfThenElse::make(ir_condition, ir_then_body);
  }
}

void CinnExprFromIslAstExpr(isl_ast_expr* node, ir::Expr* expr) {
  switch (isl_ast_expr_get_type(node)) {
    case isl_ast_expr_int: {
      isl_val* val = isl_ast_expr_get_val(node);
      *expr = ir::Expr(isl_val_get_num_si(val));
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
        CinnExprFromIslAstExpr(expr0, &op);
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
          LOG(INFO) << "** arg0 " << Dump(ops[0]);
          LOG(INFO) << "** arg1 " << Dump(ops[0]);
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

}  // namespace cinn
