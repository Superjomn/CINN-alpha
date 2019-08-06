#include "cinn/core/computation.h"
#include <gtest/gtest.h>

namespace cinn {

TEST(computation, ApplyTransformationOnScheduleDomain) {
  Computation comp;
  isl_map* init_sched = isl_map_read_from_str(comp.ctx(), "[n] -> { A[i] : 0 < i < n}");
  comp.SetSchedule(init_sched);
  comp.ApplyTransformationOnScheduleDomain("[n] -> { A[i] }");
}

}  // namespace cinn