#pragma once

#include <cinn/ir/ir.h>
#include <glog/logging.h>
#include <array>
#include <string>
#include "cinn/utils/macros.h"

namespace cinn {
namespace backends {
namespace x86 {

/**
 * Interface for X86 SIMD.
 */
class X86SIMD {
  // dtypes
  static std::array<std::string, 4> m128_dtypes;
  static std::array<std::string, 4> m256_dtypes;

  // math operators
  static std::array<std::string, 4> m128_math_op;
  static std::array<std::string, 4> m256_math_op;

  // store load operators
  static std::array<std::string, 2> m128_io_op;
  static std::array<std::string, 2> m256_io_op;

  // set1
  static std::array<std::string, 2> m128_set1_op;
  static std::array<std::string, 2> m256_set1_op;

  // reduce
  static std::array<std::string, 4> m128_custom_reduce_op;
  static std::array<std::string, 4> m256_custom_reduce_op;

  std::string* dtypes_arr_{};
  std::string* ops_arr_{};
  std::string* io_arr_{};
  std::string* set1_arr_{};
  std::string* custom_reduce_arr_{};

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

  std::string load_ps() const { return io_arr_[0]; }
  std::string store_ps() const { return io_arr_[1]; }

  std::string set1_ps() const { return set1_arr_[0]; }
  std::string set1_pd() const { return set1_arr_[1]; }

  std::string custom_reduce_add_ps() const { return custom_reduce_arr_[0]; }
  std::string custom_reduce_sub_ps() const { return custom_reduce_arr_[1]; }
  std::string custom_reduce_mul_ps() const { return custom_reduce_arr_[2]; }
  std::string custom_reduce_div_ps() const { return custom_reduce_arr_[3]; }

 private:
  int bits_;
};

static X86SIMD x86_128_simd(X86SIMD::Bits::k128);
static X86SIMD x86_256_simd(X86SIMD::Bits::k256);

const X86SIMD& GlobalX86SIMD(composite_t type);
;

/**
 * Refine the expression with necessary data cast operations such as (load, store).
 */
void RefineExprWithSIMDSupport(ir::Expr* expr);

bool IsCastSIMDReleated(const ir::Cast& cast);
std::string PrintCastOpr(const ir::Cast& cast, X86SIMD* simd, ir::NodeTy reduce_opr = ir::NodeTy::Add);

}  // namespace x86
}  // namespace backends
}  // namespace cinn
