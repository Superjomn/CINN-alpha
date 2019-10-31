#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include "cinn/core/transform/schedule_tree_writer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/string.h"

struct isl_set_cmp {
  bool operator()(const isl::set& a, const isl::set& b) {
    return strcmp(isl_set_get_tuple_name(a.get()), isl_set_get_tuple_name(b.get())) < 0;
  }
};

namespace cinn {

/**
 * Perform Tile transform in a schedule tree, currently a naive implementation, traverse the schedule tree,
 * - when reach a filter node, record the statements it filtered,
 * - when reach a band node, search the statement and dimension in the partial schedule, if found, map it to
 *   { [i] -> [i/k] }, create a new partial schedule and insert the new partial schedule.
 */
struct TileSingleDimTransformer : public ScheduleNodeRewriter<TileSingleDimTransformer> {
  using BaseTy = ScheduleNodeRewriter<TileSingleDimTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TileSingleDimTransformer(const std::string& statement, const std::string& iterator, int size)
      : statement_(statement), iterator_(iterator), tile_size_(size) {}

  /**
   * Visit a schedule band node, tile it if match the target.
   *
   * @param node the schedule node reached.
   * @return the original or updated node.
   */
  isl::schedule_node VisitBand(const isl::schedule_node& node);

  /**
   * Visit a filter node, collect the statement it filtered.
   * Question? how about replacing this with collection of the partial schedule tuple name in the band.
   * @param node the schedule node reached.
   * @return the original filter node.
   */
  isl::schedule_node VisitFilter(const isl::schedule_node& node);

 private:
  // Borrowed from llvm/Polly
  bool IsBandTileable(const isl::schedule_node& node);

  // Borrowed from llvm/Polly
  bool IsSimpleInnermostBand(const isl::schedule_node& node);

 private:
  // The statement to tile.
  std::string statement_;
  // The iterator to tile.
  std::string iterator_;
  // The tile size.
  int tile_size_;
  // A flag to avoid duplication.
  bool tiled_ = false;
};

/**
 * Tile several dimensions one time.
 */
struct TileDimsTransformer : public ScheduleNodeRewriter<TileDimsTransformer> {
  using BaseTy = ScheduleNodeRewriter<TileDimsTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TileDimsTransformer(const std::string& statement, const std::vector<int>& size, bool unroll = false)
      : statement_(statement), tile_sizes_(size), unroll_(unroll) {}

  isl::schedule_node VisitBand(const isl::schedule_node& node);

  isl::schedule_node VisitFilter(const isl::schedule_node& node) {
    CollectFilter(node);
    return Visit(node.first_child()).parent();
  }

 private:
  /**
   * Tile a schedule node.
   * @param node The node to tile.
   * @param id An name to identifies this tiling and mark it in the generated AST.
   * @param tile_sizes Specify the tile sizes.
   * @param default_tile_size A default tiling size for dimensions that are not covered by the tile_sizes vector.
   */
  isl::schedule_node TileNode(isl::schedule_node node,
                              const std::string& id,
                              const std::vector<int>& tile_sizes,
                              int default_tile_size);

 private:
  bool tiled_{false};
  std::vector<int> tile_sizes_;
  std::string statement_;
  bool unroll_;
};

/**
 * Tile the dimensions and unroll the last tile dims of a band node.
 */
struct TileUnrollTransformer : public ScheduleNodeRewriter<TileUnrollTransformer> {
  using BaseTy = ScheduleNodeRewriter<TileUnrollTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TileUnrollTransformer(const std::string& statement, const std::vector<int>& size)
      : statement_(statement), tile_sizes_(size) {}

  isl::schedule_node VisitBand(const isl::schedule_node& node);

  isl::schedule_node VisitFilter(const isl::schedule_node& node) {
    CollectFilter(node);
    return Visit(node.first_child()).parent();
  }

 private:
  bool tiled_{false};
  std::vector<int> tile_sizes_;
  std::string statement_;
};

/**
 * Unroll the last loop level.
 */
struct UnrollTransformer : public ScheduleNodeRewriter<UnrollTransformer> {
  using BaseTy = ScheduleNodeRewriter<UnrollTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  /**
   * Construct an UnrollTransformer.
   * @param statement the statement to unroll.
   * @param iterator the iterator to unroll.
   */
  UnrollTransformer(const std::string& statement, const std::string& iterator)
      : statement_(statement), iterator_(iterator) {}

  /**
   * Visit a schedule band node, tile it if match the target.
   *
   * @param node the schedule node reached.
   * @return the original or updated node.
   */
  isl::schedule_node VisitBand(const isl::schedule_node& node);

  static isl::union_set GetUnrollIsolatedSetOptions(isl::ctx ctx) {
    isl::union_set option(ctx, "{ isolate[[] -> unroll[1]] }");
    return option;
  }

  isl::union_set GetIsolateOptions(isl::set domain, int out_dims_num) {
    isl::union_set option(domain.ctx(), "{ isolate[[]->[a,b]] : 0 < a < 5 }");
    return option;
  }

 private:
  // The statement to tile.
  std::string statement_;
  // The iterator to tile.
  std::string iterator_;
  // The tile size.
  int tile_size_;
  // A flag to avoid duplication.
  bool tiled_ = false;
};

/**
 * Loop transpose on two specific iterators.
 */
struct TransposeTransformer : public ScheduleNodeRewriter<TransposeTransformer> {
  using BaseTy = ScheduleNodeRewriter<TransposeTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TransposeTransformer(const std::string& statement, const std::string& iter0, const std::string& iter1)
      : statement_(statement), iterators_(std::make_pair(iter0, iter1)) {}

  /**
   * Visit a schedule band node, tile it if match the target.
   *
   * @param node the schedule node reached.
   * @return the original or updated node.
   */
  isl::schedule_node VisitBand(const isl::schedule_node& node);

  isl::schedule_node VisitFilter(const isl::schedule_node& node);

 private:
  isl::pw_multi_aff PrepareTransform();

 private:
  // The statement to tile.
  std::string statement_;
  // The iterator to tile.
  std::pair<std::string, std::string> iterators_;
  // A flag to avoid duplication.
  bool tiled_ = false;
};

isl::schedule_node IsolateFullPartialTiles(isl::schedule_node node, int vector_width);

/**
 * Make the last dimension of set to take values from 0 to vector_width -1.
 * @param set The set to be modified.
 * @param vector_width A parameter determines the constraint.
 */
isl::set AddExtentConstraints(isl::set set, int vector_width);

/**
 * Build the set of partial tile prefixes.
 * @param schedule_range A range of a map, which describes a prefix schedule relation.
 * @param vector_width
 * @return
 */
isl::set GetPartialTilePrefixes(isl::set schedule_range, int vector_width);

/**
 * Create an isl::union_set which describes the isolate option based on isolate_domain.
 * @param isolate_domain
 * @param out_dims_num
 * @return
 */
isl::union_set GetIsolateOptions(isl::set isolate_domain, unsigned out_dims_num);

}  // namespace cinn
