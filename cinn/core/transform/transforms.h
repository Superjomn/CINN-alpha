#include <iostream>
#include <map>
#include "cinn/core/transform/schedule_tree_writer.h"
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
struct TileTransformer : public ScheduleNodeRewriter<TileTransformer> {
  using BaseTy = ScheduleNodeRewriter<TileTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TileTransformer(const std::string& statement, const std::string& iterator, int size)
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
  // The statements the latest filter collects.
  std::map<std::string, isl::set> statements_filter_collected_;
};

struct UnrollTransformer : public ScheduleNodeRewriter<UnrollTransformer> {
 private:
};

struct SkewTransformer : public ScheduleNodeRewriter<SkewTransformer> {
  using BaseTy = ScheduleNodeRewriter<SkewTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  SkewTransformer(const std::string& statement, const std::string& iterator)
      : statement_(statement), iterator_(iterator) {}

  /**
   * Visit a schedule band node, tile it if match the target.
   *
   * @param node the schedule node reached.
   * @return the original or updated node.
   */
  isl::schedule_node VisitBand(const isl::schedule_node& node);

 private:
  // The statement to tile.
  std::string statement_;
  // The iterator to tile.
  std::string iterator_;
  // A flag to avoid duplication.
  bool tiled_ = false;
  // The statements the latest filter collects.
  std::map<std::string, isl::set> statements_filter_collected_;
};

}  // namespace cinn
