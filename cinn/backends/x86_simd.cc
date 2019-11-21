#include "cinn/backends/x86_simd.h"
#include "cinn/core/cinn_context.h"

namespace cinn {
namespace backends {

std::array<std::string, 4> X86SIMD::m128_dtypes{{
    "__m128",   // floats
    "__m128d",  // doubles
    "__m128i",  // ints
    "__m128u",  // unsigned ints
}};

std::array<std::string, 4> X86SIMD::m256_dtypes{{
    "__m256",    // floats
    "__m2568d",  // doubles
    "__m2568i",  // ints
    "__m2568u",  // unsigned ints
}};

std::array<std::string, 4> X86SIMD::m128_op{{
    "_mm_add_ps",  // +
    "_mm_sub_ps",  // -
    "_mm_mul_ps",  // *
    "_mm_div_ps",  // -
}};

std::array<std::string, 4> X86SIMD::m256_op{{
    "_mm256_add_ps",  // +
    "_mm256_sub_ps",  // -
    "_mm256_mul_ps",  // *
    "_mm256_div_ps",  // /
}};

X86SIMD::X86SIMD(X86SIMD::Bits bits) {
  switch (bits) {
    case Bits::k128:
      dtypes_arr_ = m128_dtypes.data();
      ops_arr_ = m128_op.data();
      break;
    case Bits::k256:
      dtypes_arr_ = m256_dtypes.data();
      ops_arr_ = m256_op.data();
      break;
    default:
      NOT_IMPLEMENT
  }
}

}  // namespace backends

const backends::X86SIMD CINNContext::x86_simd_128{backends::X86SIMD::Bits::k128};
const backends::X86SIMD CINNContext::x86_simd_256{backends::X86SIMD::Bits::k256};

}  // namespace cinn
