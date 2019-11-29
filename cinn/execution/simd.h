#pragma once
#include <immintrin.h>

float _m128_custom_reduce_add(__m128 v);

float hsum_ps_sse3(__m128 v);

float _m256_custom_reduce_add(__m256 v);
