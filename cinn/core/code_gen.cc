#include "cinn/core/code_gen.h"
#include "cinn/ir/ir.h"
#include "code_gen.h"

namespace cinn {

// Eat an isl block node.
void EatBlock(isl_ast_node* node, ir::Expr* expr);
// Eat an isl user node.
void EatUser(isl_ast_node* node, ir::Expr* expr);
// Eat an isl for node.
void EatFor(isl_ast_node* node, ir::Expr* expr);

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
        isl_ast_expr* expr0 = isl_ast_expr_get_op_arg(node, 0);
        ExprFromIslAstExpr(expr0, &op);
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
        default:
          LOG(FATAL) << "unsupported op " << op_type;
      }
    } break;
    default:
      break;
  }
}

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
void EatUser(isl_ast_node* node, ir::Expr* expr) { CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_user); }
// Eat an isl for node.
void EatFor(isl_ast_node* node, ir::Expr* expr) {
  CHECK_EQ(isl_ast_node_get_type(node), isl_ast_node_for);

  // iter name
  isl_ast_expr* iter = isl_ast_node_for_get_iterator(node);
  ir::Var _iter(isl_ast_expr_to_C_str(iter));
  // init val
  isl_ast_expr* init = isl_ast_node_for_get_init(node);
  isl_val* init_val = isl_ast_expr_get_val(init);
}

}  // namespace cinn
