#pragma once
#include <isl/aff.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/cpp.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/union_set.h>
#include <string>
#include <vector>

namespace cinn {

//! generate an identity map, given a set.
isl_map *__isl_give isl_set_to_identity_map(__isl_keep isl_set *set);

//! Get a representation of the tuple in isl set, given a set.
//! e.g. {A[i,j] : ...} will get "A[i,j]"
std::string isl_set_to_statement_repr(__isl_keep isl_set *set);

isl_map *isl_map_add_dim_and_eq_constraint(isl_map *map, int dim_pos, int constant);

//! helper function to generate string representation for isl_set.
std::string isl_to_str(__isl_keep isl_set *);
//! helper function to generate string representation for isl_map.
std::string isl_to_str(__isl_keep isl_map *);
//! helper function to generate string representation for isl_space.
std::string isl_to_str(__isl_keep isl_space *);
//! helper function to generate string representation for isl_pw_aff.
std::string isl_to_str(__isl_keep isl_pw_aff *);
//! helper function to generate string representation for isl_union_pw_aff.
std::string isl_to_str(__isl_keep isl_union_pw_aff *);

namespace isl_utils {

isl_ast_build *__isl_give isl_ast_build_set_iterators(__isl_take isl_ast_build *build,
                                                      const std::vector<std::string> &iterators);

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

}  // namespace isl_utils
}  // namespace cinn
