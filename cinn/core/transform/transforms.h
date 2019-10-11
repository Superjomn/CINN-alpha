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

struct TileTransformer : public ScheduleNodeRewriter<TileTransformer> {
  using BaseTy = ScheduleNodeRewriter<TileTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TileTransformer(const std::string& statement, const std::string& iterator, int size)
      : statement_(statement), iterator_(iterator), tile_size_(size) {}

  //! Visit a schedule band node, tile it if match the target.
  isl::schedule_node VisitBand(const isl::schedule_node& node);

  //! Visit a filter node, collect the statement it filtered.
  // Question? how about replacing this with collection of the partial schedule tuple name in the band.
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

}  // namespace cinn
