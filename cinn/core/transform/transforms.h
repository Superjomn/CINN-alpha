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

/**
 * Tile the dimensions from the tail of a band node.
 */
struct TileTransformer2 : public ScheduleNodeRewriter<TileTransformer2> {
  using BaseTy = ScheduleNodeRewriter<TileTransformer2>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  TileTransformer2(const std::string& statement, const std::vector<int>& size)
      : statement_(statement), tile_sizes_(size) {}

  isl::schedule_node VisitBand(const isl::schedule_node& node);

  isl::schedule_node VisitFilter(const isl::schedule_node& node) {
    collected_statements_.clear();
    isl::union_set filter = isl::manage(isl_schedule_node_filter_get_filter(node.get()));
    auto collect_set = [this](isl::set x) {
      auto name = isl_set_get_tuple_name(x.get());
      collected_statements_[name] = x;
    };
    filter.foreach_set(collect_set);
    return Visit(node.first_child());
  }

 private:
  bool tiled_{false};
  std::vector<int> tile_sizes_;
  std::string statement_;
  std::map<std::string, isl::set> collected_statements_;
};

struct UnrollTransformer : public ScheduleNodeRewriter<UnrollTransformer> {
  using BaseTy = ScheduleNodeRewriter<UnrollTransformer>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  UnrollTransformer(const std::string& statement, const std::string& iterator)
      : statement_(statement), iterator_(iterator) {}

  /**
   * Visit a schedule band node, tile it if match the target.
   *
   * @param node the schedule node reached.
   * @return the original or updated node.
   */
  isl::schedule_node VisitBand(const isl::schedule_node& node);

  // Detect the statements that scheduled in a band node.
  void CollectCurrentStatementsFromBand(const isl::schedule_node& node) {
    auto domain = isl::manage(isl_schedule_node_get_domain(node.get()));
    auto band = node.as<isl::schedule_node_band>();
    auto partial_schedule = band.get_partial_schedule();
    LOG(INFO) << "partial schedule: " << partial_schedule;
    LOG(INFO) << "domain: " << domain;
  }

  static isl::union_set GetUnrollIsolatedSetOptions(isl::ctx ctx) {
    isl::union_set option(ctx, "{ isolate[[] -> unroll[1]] }");
    return option;
    /*
    auto space = isl::manage(isl_space_alloc(ctx.get(), 0, 0, 1));
    isl::map option = isl::map::universe(space);
    isl::id dim_in_id = isl::manage(isl_id_alloc(ctx.get(), "isolate", nullptr));
    isl::id dim_out_id = isl::manage(isl_id_alloc(ctx.get(), "unroll", nullptr));

    option = isl::manage(isl_map_set_tuple_id(option.release(), isl_dim_in, dim_in_id.release()));
    option = isl::manage(isl_map_set_tuple_id(option.release(), isl_dim_out, dim_out_id.release()));
    return isl::manage(isl_map_wrap(option.release()));
     */
  }

  isl::union_set GetIsolateOptions(isl::set domain, int out_dims_num) {
    isl::union_set option(domain.ctx(), "{ isolate[[]->[a,b]] : 0 < a < 5 }");
    return option;
    /*
    unsigned dims = isl_set_dim(domain.get(), isl_dim_set);
    CHECK_LE(out_dims_num, dims);
    isl::map isolate_relation = isl::manage(isl_map_from_domain(domain.release()));
    isolate_relation = isl::manage(
        isl_map_move_dims(isolate_relation.release(), isl_dim_out, 0, isl_dim_in, dims - out_dims_num, out_dims_num));
    isl::set isolate_option = isl::manage(isl_map_wrap(isolate_relation.release()));
    isl::id id = isl::manage(isl_id_alloc(isolate_option.ctx().get(), "isolate", nullptr));
    isolate_option = isl::manage(isl_set_set_tuple_id(isolate_option.release(), id.release()));
    return isl::union_set(isolate_option);
     */
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
  // The statements the latest filter collects.
  std::map<std::string, isl::set> statements_filter_collected_;
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
