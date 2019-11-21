#pragma once

#include <glog/logging.h>
#include <array>
#include <string>
#include "cinn/utils/macros.h"

namespace cinn {
namespace backends {

/**
 * Interface for X86 SIMD.
 */
class X86SIMD {
  // dtypes
  static std::array<std::string, 4> m128_dtypes;
  static std::array<std::string, 4> m256_dtypes;

  // operators
  static std::array<std::string, 4> m128_op;
  static std::array<std::string, 4> m256_op;

  std::string* dtypes_arr_{};
  std::string* ops_arr_{};

 public:
  enum Bits {
    k128,
    k256,
    k512,
  };

  explicit X86SIMD(Bits bits);

  std::string packed_float_t() const { return dtypes_arr_[0]; }
  std::string packed_double_t() const { return dtypes_arr_[1]; }
  std::string packed_int_t() const { return dtypes_arr_[2]; }
  std::string pack_unsigned_t() const { return dtypes_arr_[3]; }

  std::string add_ps() const { return ops_arr_[0]; }
  std::string sub_ps() const { return ops_arr_[1]; }
  std::string mul_ps() const { return ops_arr_[2]; }
  std::string div_ps() const { return ops_arr_[3]; }

 private:
  int bits_;
};

}  // namespace backends
}  // namespace cinn
