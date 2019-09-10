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

class map : public isl::map {
 public:
  inline /* implicit */ map(const map &obj) : isl::map(obj) {}
  inline /* implicit */ map(isl::basic_map bmap) : isl::map(bmap) {}
  inline explicit map(isl::ctx ctx, const std::string &str) : isl::map(ctx, str) {}

  isl::map project_out(isl_dim_type dim_type, int start, int n) {
    return isl::manage(isl_map_project_out(copy(), dim_type, start, n));
  }
};

}  // namespace isl_utils

}  // namespace cinn
