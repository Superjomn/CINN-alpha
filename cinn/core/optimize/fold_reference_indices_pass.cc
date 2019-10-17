#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

template <typename T>
std::string DumpIR(const T &x) {
  std::stringstream ss;
  ir::IRPrinter printer(ss);
  printer.Visit(x);
  return ss.str();
}

namespace {

//! Record the pieces and counter in a block.
struct BlockPieceStatis {
  //! The pieces of expression of indices in a Reference.
  std::map<std::string, Expr> pieces;
  //! Record the piece's frequency.
  std::map<std::string, int> counter;
  //! The new let expressions for the temporary variables' declaration.
  std::map<std::string, Expr> let_exprs;

  void KeepARecord(const Expr &piece) {
    auto key = DumpIR(&piece);
    pieces[key] = piece;
    counter[key]++;
  }
};

using statis_t = std::map<const ir::Block *, BlockPieceStatis>;

/**
 * Collect the pieces of indice expressions from Reference node.
 */
class ReferenceIndicesPiecesCollector : public ir::IRMutator {
  bool inside_reference_{false};
  statis_t &statis_;
  ir::Block *cur_block_{};

 public:
  ReferenceIndicesPiecesCollector(statis_t &statis) : statis_(statis) {}

  void Visit(const Expr *op, Expr *expr) override { IRMutator::Visit(op, expr); }

 protected:
#define OP_2PARAM(op__)                                 \
  void Visit(const ir::op__ *op, Expr *expr) override { \
    CHECK(op);                                          \
    if (inside_reference_) {                            \
      statis_[cur_block_].KeepARecord(*expr);           \
    }                                                   \
    auto *a = expr->As<ir::op__>();                     \
    Visit(&a->a, &a->a);                                \
    Visit(&a->b, &a->b);                                \
  }
#define OP_1PARAM(op__)                                 \
  void Visit(const ir::op__ *op, Expr *expr) override { \
    CHECK(op);                                          \
    if (inside_reference_) {                            \
      statis_[cur_block_].KeepARecord(*expr);           \
    }                                                   \
    auto *a = expr->As<ir::op__>();                     \
    Visit(&a->a, &a->a);                                \
  }
  OP_1_ARGS_FOR_EACH(OP_1PARAM);
  OP_2_ARGS_FOR_EACH(OP_2PARAM);

#undef OP_1PARAM
#undef OP_2PARAM

  void Visit(const ir::Reference *op, Expr *expr) override {
    EnterReference();

    auto *a = expr->As<ir::Reference>();
    for (auto &iter : a->iterators) {
      Visit(&iter, &iter);
    }

    ExitReference();
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *a = expr->As<ir::Block>();
    cur_block_ = a;

    ir::IRMutator::Visit(op, expr);
  }

  void EnterReference() { inside_reference_ = true; }
  void ExitReference() { inside_reference_ = false; }
};

/**
 * Detect the pattern of "A[e1] = A[e1] + ... and compose it into some thing like A[e1] += ..., works with other
 * operators such as - * /.
 */
class Mutator : public ir::IRMutator {
  statis_t statis;
  bool inside_reference_{false};
  ir::Block *cur_block{};

 public:
  Mutator(const statis_t &statis) : statis(statis) {}

  void Visit(const ir::Expr *op, ir::Expr *expr) override { ir::IRMutator::Visit(op, expr); }

  void BlockPrependLetExprs(ir::Block *cur_block) {
    auto it = statis.find(cur_block);
    // The block of Function or other special blocks have no lets.
    if (it != statis.end()) {
      for (auto &let_expr_item : it->second.let_exprs) {
        LOG(INFO) << "pre append lets: " << let_expr_item.first << " " << DumpIR(&let_expr_item.second);
        cur_block->exprs.insert(std::begin(cur_block->exprs), let_expr_item.second);
      }
    }
  }

#define OP_2PARAM(op__)                                                                      \
  void Visit(const ir::op__ *op, Expr *expr) override {                                      \
    CHECK(op);                                                                               \
    if (inside_reference_) {                                                                 \
      const auto &block_statis = statis[cur_block];                                          \
      std::string key = DumpIR(op);                                                          \
      auto it = block_statis.pieces.find(key);                                               \
      if (it != block_statis.pieces.end()) {                                                 \
        LOG(INFO) << cur_block << " replace " << it->first << " to " << DumpIR(&it->second); \
        *expr = ir::CopyExpr(it->second);                                                    \
        return;                                                                              \
      }                                                                                      \
    }                                                                                        \
    auto *a = expr->As<ir::op__>();                                                          \
    Visit(&a->a, &a->a);                                                                     \
    Visit(&a->b, &a->b);                                                                     \
  }
#define OP_1PARAM(op__)                                     \
  void Visit(const ir::op__ *op, Expr *expr) override {     \
    CHECK(op);                                              \
    if (inside_reference_) {                                \
      std::string key = DumpIR(op);                         \
      LOG(INFO) << "checking " << key;                      \
      const auto &block_statis = statis[cur_block];         \
      auto it = block_statis.pieces.find(key);              \
      if (it != block_statis.pieces.end()) {                \
        LOG(INFO) << cur_block << " replace " << it->first; \
        *expr = ir::CopyExpr(it->second);                   \
        return;                                             \
      }                                                     \
    }                                                       \
    auto *a = expr->As<ir::op__>();                         \
    Visit(&a->a, &a->a);                                    \
  }
  OP_1_ARGS_FOR_EACH(OP_1PARAM);
  OP_2_ARGS_FOR_EACH(OP_2PARAM);

  void Visit(const ir::Reference *op, Expr *expr) override {
    inside_reference_ = true;
    // %{
    auto *a = expr->As<ir::Reference>();
    for (auto &iter : a->iterators) {
      Visit(&iter, &iter);
    }
    // %}
    inside_reference_ = false;
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *a = expr->As<ir::Block>();
    cur_block = a;
    BlockPrependLetExprs(a);

    ir::IRMutator::Visit(op, expr);
  }
};

}  // namespace

class ThePass : public Pass {
  statis_t statis;
  const int frequency{10};

 public:
  explicit ThePass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    ReferenceIndicesPiecesCollector collector(statis);
    collector.Visit(expr, expr);

    LOG(INFO) << "collected " << statis.size() << " blocks";
    FoldHighFrequencyPiece();

    Mutator mutator(statis);
    mutator.Visit(expr, expr);
  }

  void FoldHighFrequencyPiece() {
    for (auto &block_item : statis) {
      LOG(INFO) << "block " << block_item.first << " origin " << block_item.second.pieces.size();
      for (auto &item : block_item.second.counter) {
        if (item.second >= frequency) {
          if (!block_item.second.let_exprs.count(item.first)) {
            std::string name = NameGenerator::Global().NewVarName();
            auto b = block_item.second.pieces[item.first];
            ir::Var var(name, b.ptype());
            block_item.second.let_exprs[item.first] = ir::Let::make(Expr(var), b);
            block_item.second.pieces[item.first] = Expr(var);
          }
        } else {
          block_item.second.pieces.erase(item.first);
        }
      }
      LOG(INFO) << "--  " << block_item.second.pieces.size();
    }
  }
};

}  // namespace cinn

REGISTER_PASS(fold_reference_indices, cinn::ThePass);
