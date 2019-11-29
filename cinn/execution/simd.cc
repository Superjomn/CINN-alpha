#include "simd.h"  // NOLINT

float _m128_custom_reduce_add(__m128 v) {
  __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
  __m128 sums = _mm_add_ps(v, shuf);
  shuf = _mm_movehl_ps(shuf, shuf);
  sums = _mm_add_ss(sums, shuf);
  return _mm_cvtss_f32(sums);
}

float hsum_ps_sse3(__m128 v) {
  __m128 shuf = _mm_movehdup_ps(v);
  __m128 sums = _mm_add_ps(v, shuf);
  shuf = _mm_movehl_ps(shuf, sums);
  sums = _mm_add_ss(sums, shuf);
  return _mm_cvtss_f32(sums);
}

float _m256_custom_reduce_add(__m256 v) {
  __m128 vlow = _mm256_castps256_ps128(v);
  __m128 vhigh = _mm256_extractf128_ps(v, 1);
  vlow = _mm_add_ps(vlow, vhigh);
  return hsum_ps_sse3(vlow);
}
