#include "simd.h"  // NOLINT
#include <glog/logging.h>
#include <gtest/gtest.h>

TEST(simd256, add) {
  float* data = static_cast<float*>(aligned_alloc(32, 8 * sizeof(float)));
  for (int i = 0; i < 8; i++) data[i] = i * 0.1f;

  auto v = _mm256_load_ps(data);

  float sum = _m256_custom_reduce_add(v);
  LOG(INFO) << sum;
  ASSERT_NEAR(sum, 2.8f, 1e-5);

  delete data;
}
