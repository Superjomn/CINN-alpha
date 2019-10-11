#include "cinn/core/transform/schedule_tree_writer.h"
#include <gtest/gtest.h>

namespace cinn {

isl_bool handler(isl_schedule_node* schedule, void* user) {
  switch (isl_schedule_node_get_type(schedule)) {
    case isl_schedule_node_band:
      std::cout << "band" << std::endl;
      break;
    case isl_schedule_node_filter:
      std::cout << "filter" << std::endl;
      break;
    case isl_schedule_node_domain:
      std::cout << "domain" << std::endl;
      break;
    default:
      break;
  }

  return isl_bool_true;
}

TEST(schedule_tree_writer, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();
  LOG(INFO) << "schedule: " << schedule;

  ApplyASTBuildOptions appler;
  appler.Visit(schedule);

  isl_schedule_foreach_schedule_node_top_down(schedule.get(), handler, nullptr);
}

}  // namespace cinn
