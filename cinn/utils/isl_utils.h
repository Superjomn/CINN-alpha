#pragma once
#include <glog/logging.h>
#include <isl/aff.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/cpp.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/space.h>
#include <isl/union_set.h>
#include <map>
#include <string>
#include <vector>
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/string.h"

namespace cinn {

//! generate an identity map, given a set.
isl_map *__isl_give isl_set_to_identity_map(__isl_keep isl_set *set);

//! Get a representation of the tuple in isl set, given a set.
//! e.g. {A[i,j] : ...} will get "A[i,j]"
std::string isl_set_to_statement_repr(__isl_keep isl_set *set);

//! Get a representation of the tuple in the map.
std::string isl_map_get_statement_repr(__isl_keep isl_map *map, isl_dim_type type);

//! Get a representation of the tuple in the set.
std::string isl_set_get_statement_repr(__isl_keep isl_set *set);

//! Get a dimension position if it match the name. return -1 if not exists.
int isl_map_get_dim_pos_by_name(__isl_keep isl_map *map, isl_dim_type type, const std::string &name);

std::vector<std::string> isl_map_get_dim_names(isl_map *map, isl_dim_type type);

isl_map *isl_map_add_dim_and_eq_constraint(isl_map *map, int dim_pos, int constant);

//! helper function to generate string representation for isl_set.
std::string isl_to_str(__isl_keep isl_set *);
//! helper function to generate string representation for isl_map.
std::string isl_to_str(__isl_keep isl_map *);
//! helper function to generate string representation for isl_space.
std::string isl_to_str(__isl_keep isl_space *);

isl_map *__isl_give isl_map_set_dim_names(isl_map *__isl_give map,
                                          isl_dim_type type,
                                          const std::vector<std::string> &names);

isl_ast_build *__isl_give isl_ast_build_set_iterators(__isl_take isl_ast_build *build,
                                                      const std::vector<std::string> &iterators);

__isl_give isl_schedule_node *tile_band(__isl_take isl_schedule_node *node, __isl_take isl_multi_val *sizes);

/**
 * A helper to pass argument to caller functions in traversing schedule tree.
 */
struct IslTileGenerator {
  static IslTileGenerator &Global() {
    static std::unique_ptr<IslTileGenerator> x(new IslTileGenerator);
    return *x;
  }

  void set_stage_name(const std::string &name) { stage_name_ = name; }
  const std::string &stage_name() const { return stage_name_; }

  void ResetScheduleFilter() { schedule_filter_.reset(nullptr); }

  void set_schedule_filter(const isl::set &x) { schedule_filter_.reset(new isl::set(x)); }

  std::string schedule_filter_tuple() const {
    return schedule_filter_ ? isl_set_get_tuple_name(schedule_filter_->get()) : "";
  }

  bool schedule_filter_match_stage() const { return stage_name() == schedule_filter_tuple(); }

  void set_tiles(const std::map<std::string, int> &tiles) { tiles_ = tiles; }

  // TODO(Superjomn) many bugs.
  //! Get the tile block size for the current stage.
  isl::multi_val GetTileSizes(isl_space *space) {
    CHECK(schedule_filter_);
    LOG(INFO) << "filter: " << *schedule_filter_;
    LOG(INFO) << "space: " << isl_space_to_str(space);
    // get multi_val initialization from filter set.
    isl::multi_val sizes = isl::manage(isl_multi_val_zero(isl_space_copy(space)));
    sizes = sizes.add(32);
    LOG(INFO) << "sizes: " << sizes;

    /*
    for (int i = 0; i < isl_space_dim(space, isl_dim_set); i++) {
      auto *dim_name = isl_space_get_dim_name(space, isl_dim_set, i);
      auto it = tiles_.find(dim_name);
      if (it != tiles_.end()) {
        isl_val *val = isl_val_int_from_si(isl_space_get_ctx(space), it->second);
        sizes = isl::manage(isl_multi_val_set_at(sizes.release(), i, val));
      }
    }
     */
    return sizes;
  }

  isl_set *schedule_filter() const { return schedule_filter_ ? schedule_filter_->get() : nullptr; }

 private:
  IslTileGenerator() = default;
  std::map<std::string, int> tiles_;
  std::string stage_name_;
  std::unique_ptr<isl::set> schedule_filter_;
};

// A callback for tile a schedule node.
// TODO(Superjomn) it is weak, enhance it.
isl_schedule_node *node_tiler(__isl_take isl_schedule_node *node, void *user);

namespace isl_utils {

isl_union_map *__isl_give isl_calculate_dependency(__isl_take isl_union_map *s0_reads,
                                                   __isl_take isl_union_map *s0_writes,
                                                   __isl_take isl_union_map *s1_reads,
                                                   __isl_take isl_union_map *s1_writes);

std::string isl_space_get_statement_repr(isl_space *__isl_keep space);

class map : public isl::map {
 public:
  inline map() : isl::map() {}
  inline /* implicit */ map(const map &obj) : isl::map(obj) {}
  inline /* implicit */ map(isl::basic_map bmap) : isl::map(bmap) {}
  inline explicit map(isl::ctx ctx, const std::string &str) : isl::map(ctx, str) {}
  inline map(isl::map &&map) { ptr = map.release(); }

  int domain_dims() const { return isl_map_dim(ptr, isl_dim_in); }
  int range_dims() const { return isl_map_dim(ptr, isl_dim_out); }

  const char *domain_dim_name(int i) const { return isl_map_get_dim_name(ptr, isl_dim_in, i); }
  const char *range_dim_name(int i) const { return isl_map_get_dim_name(ptr, isl_dim_out, i); }

  bool domain_has_dim_name(int i) const { return isl_map_has_dim_name(ptr, isl_dim_in, i); }
  bool range_has_dim_name(int i) const { return isl_map_has_dim_name(ptr, isl_dim_out, i); }

  void domain_set_dim_name(int i, const char *name) { isl_map_set_dim_name(ptr, isl_dim_in, i, name); }
  void range_set_dim_name(int i, const char *name) { isl_map_set_dim_name(ptr, isl_dim_out, i, name); }

  isl::map project_out(isl_dim_type dim_type, int start, int n) {
    return isl::manage(isl_map_project_out(copy(), dim_type, start, n));
  }

  void set_in_tuple_name(const std::string &name) { ptr = isl_map_set_tuple_name(ptr, isl_dim_in, name.c_str()); }
  void set_out_tuple_name(const std::string &name) { ptr = isl_map_set_tuple_name(ptr, isl_dim_out, name.c_str()); }
};

class union_map : public isl::union_map {
 public:
  inline union_map() : isl::union_map() {}
  inline /* implicit */ union_map(const isl::union_map &obj) : isl::union_map(obj) {}
  inline /* implicit */ union_map(isl::basic_map bmap) : isl::union_map(bmap) {}
  inline /* implicit */ union_map(isl::map map) : isl::union_map(map) {}
  inline explicit union_map(isl::ctx ctx, const std::string &str) : isl::union_map(ctx, str) {}

  void add_map_inplace(isl::map &&m) { ptr = isl_union_map_add_map(ptr, m.release()); }
  union_map add_map(isl::map &&m) const { return isl::manage(isl_union_map_add_map(copy(), m.release())); }

  void union_inplace(isl::union_map &&m) { ptr = isl_union_map_union(ptr, m.release()); }
};

static isl_ctx *global_isl_ctx() {
  thread_local isl_ctx *x = isl_ctx_alloc();
  return x;
}

static std::string DumpSchedule(isl_ctx *ctx, const isl::schedule &schedule) {
  isl_printer *printer = isl_printer_to_str(ctx);
  printer = isl_printer_set_yaml_style(printer, ISL_YAML_STYLE_BLOCK);
  printer = isl_printer_print_schedule(printer, schedule.get());
  return isl_printer_get_str(printer);
}

//! Check whether the set has the dimension having a specific name.
bool isl_set_has_dim_name(isl_set *__isl_keep set, const std::string &name);

//! Check whether the map has the dimension having a specific name.
bool isl_map_has_dim_name(isl_map *__isl_keep map, isl_dim_type type, const std::string &name);

}  // namespace isl_utils
}  // namespace cinn
