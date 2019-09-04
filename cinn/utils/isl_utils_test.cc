#include "cinn/utils/isl_utils.h"
#include <glog/logging.h>
#include <gtest/gtest.h>

namespace cinn {

TEST(isl_set_to_identity_map, basic) {
  isl_ctx* ctx = isl_ctx_alloc();
  LOG(INFO) << "get ctx";
  isl_set* set = isl_set_read_from_str(ctx, "{A[i,j] : 0<i<100 and 10 < j < 101}");
  LOG(INFO) << "get set " << isl_set_to_str(set);
  isl_map* map = isl_set_to_identity_map(set);
  // LOG(INFO) << "map: " << isl_map_to_str(map);
}

}  // namespace cinn
