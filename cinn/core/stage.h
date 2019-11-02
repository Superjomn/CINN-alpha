#pragma once
#include <gtest/gtest_prod.h>
#include <isl/aff.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/cpp.h>
#include <isl/ctx.h>
#include <isl/id.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include "cinn/ir/ir.h"

#include "cinn/core/buffer.h"

namespace cinn {

/**
 * Stage is an statement.
 */
class Stage {
 public:
  enum class Type {
    unk = -1,
    polyhedral,
    function_call,
  };

  static bool is_unk(Type x) { return x == Type::unk; }
  static bool is_polyhedral(Type x) { return x == Type::polyhedral; }
  static bool is_function_call(Type x) { return x == Type::function_call; }

  struct Data {
    // ISL context.
    isl_ctx* ctx{};

    // Iteration domain.
    isl::set iter_domain;

    ir::Expr expr;

    // The schedules of this computation.
    isl_utils::map schedule;

    isl::union_map read_access;
    isl::union_map write_access;

    // Name of this computation.
    std::string name;

    // statements' indice map from CINN to isl ast.
    std::map<std::string, ir::Expr> indice_map_;

    static std::set<std::string> names;

    // Var name to tile size.
    std::map<std::string, int> tiles;

    // Tile from the tail.
    std::vector<int> tile_sizes_;

    // The size to vectorize.
    int vector_width_{-1};

    bool unroll_{false};

    //! the dimensions to transpose.
    std::vector<std::pair<std::string, std::string>> transposes_;

    // The names of the stages try to fuse with.
    std::set<std::string> stages_fuse_with;
  };

  //! The iterators in order, the statement. It is used only once in the ExtractDomainFromExpr, so it is not in data_.
  std::vector<ir::Var> iterators_in_order_;

 public:
  Stage() { InitData(); }
  Stage(const std::string& name, const std::string& iter_domain);

  /**
   * Create a Stage using an Expr (should be an Reference node).
   *
   * For example
   *
   *     Var i("i", 0, 100);
   *     Expr A, B;
   *     B(i) = A(i) + 1;
   *
   *     Stage s0(B);
   *
   * One can specify the iterators of the statement in the schedule by passing the argument iterators.
   */
  Stage(ir::Expr expr, const std::vector<ir::Var>& iterators = {});

  Stage(const std::shared_ptr<Stage::Data>& x) : data_(x) {}

  // Stage is free to copy.
  Stage(const Stage& stage) : data_(stage.data_) {}
  Stage(Stage&& stage) : data_(std::move(stage.data_)) {}

  void operator=(const Stage& other) { data_ = other.data_; }

  /// Get the expression this stage holds.
  const ir::Expr& expr() const { return data_->expr; }

  //! Get the node type.
  Type type() const;

  /**
   * Set extra condition to the iteration domain.
   * @param iterator the iterator to set the condition.
   * @param cond the string represented condition of this iterator.
   *
   *
   * Such as
   *
   * s0.SetCond(i, " %2 = 0"); // means the condition is "i % 2 = 0".
   */
  void SetCond(const ir::Var& iterator, const std::string& cond);

  /**
   * Set extra condition to the iteration domain.
   * @param expr the expression as the iterator in a reference.
   * @param cond the string represented condition of this iterator.
   *
   *
   * Such as
   *
   * s0.SetCond(i*Expr(2), " %2 = 0"); // means the condition is "i % 2 = 0".
   */
  void SetCond(const ir::Expr& expr, const std::string& cond);

  //! Tell whether this stage is an Assign stage.
  bool is_assign() const;
  //! Tell whether this stage is an Allocate stage.
  bool is_allocate() const;

  //! Get the isl ctx.
  isl_ctx* ctx() {
    CHECK(data_->ctx);
    return data_->ctx;
  }
  //! Get the iteration domain of the expression this stage holds.
  const isl::set& iterator_domain() const { return data_->iter_domain; }

  //! Set name of the stage.
  void set_name(const std::string& name);
  //! Get the name of the stage.
  const std::string& name() const { return data_->name; }

  isl_union_map* read_access() const { return data_->read_access.get(); }
  void set_read_access(const isl::union_map& other) { data_->read_access = other; }
  isl_union_map* write_access() const { return data_->write_access.get(); }
  void set_write_access(const isl::union_map& other) { data_->write_access = other; }

  const isl_utils::map& schedule() const { return data_->schedule; }

  const std::map<std::string, int> tiles() const { return data_->tiles; }

  const std::vector<int>& tile_sizes() const { return data_->tile_sizes_; }

  const std::vector<std::pair<std::string, std::string>>& transposes() const { return data_->transposes_; }

  int vector_width() const { return data_->vector_width_; }

  const std::set<std::string>& stages_fuse_with() const { return data_->stages_fuse_with; }

  //! Set the extra condition of the iterators.
  void SetCond(const std::string& x);

  // Some basic polyhedral transformations --------------------------------------------------------

  void FuseWith(const Stage& o) { data_->stages_fuse_with.insert(o.name()); }

  //! Interchange two loop levels `i` and `j`.
  void Interchange(ir::Var i, ir::Var j);

  //! Interchange two loop levels named `i` and `j`.
  void Interchange(const std::string& dim0, const std::string& dim1);

  //! Tile in `i` iterator by `iw` stride, `j` iterator by `jw` stride.
  // void Tile(ir::Var i, size_t iw, ir::Var j, size_t jw);
  void Tile(ir::Var i, size_t w);

  /**
   * Tile the last several loop levels with tile sizes set by `sizes`.
   * @param sizes the sizes to tile.
   */
  void Tile(const std::vector<int>& sizes);

  /**
   * Tile the forloop and unroll the last level.
   * @param sizes the tile sizes.
   */
  void TileUnroll(const std::vector<int>& sizes) {
    data_->unroll_ = true;
    data_->tile_sizes_ = sizes;
  }

  //! Skew in the loop level `i`.
  void Skew(ir::Var i);

  //! Reverse the loop level `i`.
  void Reverse(ir::Var i);

  //! Vectorize the loop level `i`.
  void Vectorize(size_t vector_size);

  // After transformations.
  isl::map GetTransformedSchedule();

  Stage& operator=(ir::Expr x) {
    InitFromAssignExpr(x);
    return *this;
  }

  bool unroll() const { return data_->unroll_; }

  // Dump the schedule to ISL C code.
  std::string DumpIslC() const;

  // Dump to C-like code.
  std::string DumpAsC() const;

  void SetIndiceMap(std::map<std::string, ir::Expr>&& indice_map) { data_->indice_map_ = std::move(indice_map); }
  const std::map<std::string, ir::Expr>& indice_map() const { return data_->indice_map_; }

 private:
  void InitData();
  // Init schedule with identity schedule.
  void InitSchedule();
  //! Name all the dimensions of the schedule if not named.
  void ScheduleNameAllDims();
  //! Initialize the stage from an assign expression, It will extract the domain and schedule information from the
  //! expression.
  void InitFromAssignExpr(ir::Expr x);
  //! Initialize the stage from an allocate expression.
  void InitFromAllocateExpr(ir::Expr x);
  //! Extract the iterator domain from the expr.
  // e.g. given the expression: A(i,j) = B(i,j) + C(i,k+1); 0<=i,j,k<=N, the iterator domain is
  // [N]->{ S0[i,j,k]: 0<=i,j,k<=N }
  void ExtractDomainFromExpr(ir::Expr x);
  //! Init the read dependencies.
  void InitReadDependencies();
  //! Init the read dependencies.
  void InitWriteDependencies();
  /**
   * Initalize the stage's buffer access(write).
   * Each stage associates to a buffer by default, for examle:
   *
   *   Buffer buf0;
   *   Expr A("A");
   *   A.Attach(buf0);
   *
   * all expression A's dependency also associate to buf0.
   */
  void InitBufferDependency();
  //! Interchange the i-th and j-th loop level.
  void Interchange(int pos0, int pos1);

  //! Split the loop level having the iterator `i` into two new loop levels, the dimension of the inner level is size.
  void Split(const ir::Var& iter, int size);

  // Tests
  FRIEND_TEST(Stage, Interchange);
  FRIEND_TEST(Stage, Split);

  friend class Generator;

 private:
  std::shared_ptr<Data> data_;
};

std::ostream& operator<<(std::ostream& os, Stage::Type t);

}  // namespace cinn
