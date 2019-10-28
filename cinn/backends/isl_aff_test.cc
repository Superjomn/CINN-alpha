#include <gtest/gtest.h>
#include "cinn/utils/isl_utils.h"

TEST(aff, aff) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::aff aff1(ctx, "{ [x,y] -> [2x] }");
  isl::aff aff2(ctx, "{ [x,y] -> [y] }");

  LOG(INFO) << "aff1 " << aff1;
  LOG(INFO) << "aff2 " << aff2;
  auto aff_add = aff1.add(aff2);
  LOG(INFO) << "aff_add: " << aff_add;
  LOG(INFO) << "aff1.add_constant 2 " << aff1.add_constant(2);
  LOG(INFO) << "aff1.ceil " << aff1.ceil();
  LOG(INFO) << "aff1.pullback " << aff1.pullback(isl::multi_aff(ctx, "{ A[i,j] -> [i,j] }"));
}

TEST(aff, pw_aff) {
  isl::ctx ctx(isl_ctx_alloc());
  // isl::pw_aff aff(ctx, "");
}

// Apply some transformations.
TEST(aff, multi_union_pw_aff) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::multi_union_pw_aff aff(ctx, "[{ A[x,y,z] -> [4 * floor(x/4)] },  { A[x,y,z] -> [y] }, { A[x,y,z] -> [z] }]");
  isl::pw_multi_aff pma(ctx, "{ [a,b,c] -> [c,b,a] }");
  auto applied = isl::manage(isl_multi_union_pw_aff_apply_pw_multi_aff(aff.release(), pma.release()));
  LOG(INFO) << "applied: " << applied;
}
