#include "cinn/core/transform/transforms.h"
#include <gtest/gtest.h>

namespace cinn {

TEST(tile, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();
  LOG(INFO) << "schedule: " << schedule;

  TileTransformer appler("A", "i", 32);
  std::string target_schedule =
      "{ domain: \"{ B[i, j] : 0 < i <= 99 and 0 < j <= 99; A[i, j] : 0 < i <= 199 and 0 < j <= 199 }\", child: { "
      "sequence: [ { filter: \"{ A[i, j] }\", child: { schedule: \"[{ A[i, j] -> [(floor((i)/32))] }, { A[i, j] -> "
      "[(j)] }]\", child: { schedule: \"[{ A[i, j] -> [(i)] }, { A[i, j] -> [(j)] }]\", permutable: 1, coincident: [ "
      "1, 1 ] } } }, { filter: \"{ B[i, j] }\", child: { schedule: \"[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]\", "
      "permutable: 1, coincident: [ 1, 1 ] } } ] } }";
  schedule = appler.Visit(schedule).get_schedule();

  LOG(INFO) << "schedule: " << schedule;
  ASSERT_EQ(GetStreamStr(schedule), target_schedule);
}

}  // namespace cinn