#include "cinn/backends/x86_simd.h"
#include "cinn/core/cinn_context.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"

namespace cinn {
namespace backends {
namespace x86 {

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

std::array<std::string, 6> X86SIMD::m128_math_op{{
    "_mm_add_ps",  // +
    "_mm_sub_ps",  // -
    "_mm_mul_ps",  // *
    "_mm_div_ps",  // -
    "_mm_max_ps",  // max
    "_mm_min_ps",  // min
}};

std::array<std::string, 6> X86SIMD::m256_math_op{{
    "_mm256_add_ps",  // +
    "_mm256_sub_ps",  // -
    "_mm256_mul_ps",  // *
    "_mm256_div_ps",  // /
    "_mm256_max_ps",  // max
    "_mm256_min_ps",  // min
}};

std::array<std::string, 2> X86SIMD::m128_io_op{{
    "_mm_load_ps",   //
    "_mm_store_ps",  //
}};

std::array<std::string, 2> X86SIMD::m256_io_op{{
    "_mm256_load_ps",   //
    "_mm256_store_ps",  //

}};

std::array<std::string, 2> X86SIMD::m128_set1_op{{
    "_mm_set1_ps",  //
    "_mm_set1_pd",  //
}};
std::array<std::string, 2> X86SIMD::m256_set1_op{{
    "_mm256_set1_ps",  //
    "_mm256_set1_pd",  //
}};

std::array<std::string, 4> X86SIMD::m128_custom_reduce_op{{
    "_m128_custom_reduce_add",  //
    "_m128_custom_reduce_sub",  //
    "_m128_custom_reduce_mul",  //
    "_m128_custom_reduce_div",  //
}};
std::array<std::string, 4> X86SIMD::m256_custom_reduce_op{{
    "_m256_custom_reduce_add",  //
    "_m256_custom_reduce_sub",  //
    "_m256_custom_reduce_mul",  //
    "_m256_custom_reduce_div",  //
}};

X86SIMD::X86SIMD(X86SIMD::Bits bits) {
  switch (bits) {
    case Bits::k128:
      dtypes_arr_ = m128_dtypes.data();
      ops_arr_ = m128_math_op.data();
      set1_arr_ = m128_set1_op.data();
      io_arr_ = m128_io_op.data();
      custom_reduce_arr_ = m128_custom_reduce_op.data();
      break;
    case Bits::k256:
      dtypes_arr_ = m256_dtypes.data();
      ops_arr_ = m256_math_op.data();
      set1_arr_ = m256_set1_op.data();
      io_arr_ = m256_io_op.data();
      custom_reduce_arr_ = m256_custom_reduce_op.data();
      break;
      break;
    default:
      NOT_IMPLEMENT
  }
}

void RefineExprWithSIMDSupport(ir::Expr *expr) {
  struct Mutator : public ir::IRMutator {
    void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
    void Visit(const ir::SIMDOpr *op, ir::Expr *expr) override {
      switch (op->opr) {}

      IRMutator::Visit(op, expr);
    }
  };
}

bool IsCastSIMDReleated(const ir::Cast &cast) { return cast.is_simd() || cast.expr.is_simd(); }

std::string PrintCastOpr(const ir::Cast &cast, X86SIMD *simd, ir::NodeTy reduce_opr) {
  if (cast.is_simd()) {
    CHECK(cast.expr.is_primitive()) << cast.expr;
    std::vector<std::string> ids;
    auto identity = cast.expr.As<ir::Identity>();
    Expr arg = identity ? identity->GetTrimedExpr(&ids) : cast.expr;

    CHECK(arg.ptype() == primitive_t::float32) << "just support fp32";

    bool is_pointer = Found(ids, std::string(expr_ids::reference_address));

    if (!is_pointer)
      return simd->set1_ps();
    else
      return simd->load_ps();

  } else if (cast.is_primitive() && cast.expr.is_simd()) {
    std::string opr_repr;
    switch (reduce_opr) {
      case ir::NodeTy::Add:
        opr_repr = "add";
        break;
      case ir::NodeTy::Sub:
        opr_repr = "sub";
        break;
      case ir::NodeTy::Mul:
        opr_repr = "mul";
        break;
      case ir::NodeTy::Div:
        opr_repr = "div";
        break;
      default:
        NOT_IMPLEMENT
    }
    return "_" + GetStreamStr(cast.expr.ctype()) + "_reduce_" + opr_repr;
  }
}

const X86SIMD &GlobalX86SIMD(composite_t type) {
  switch (type) {
    case composite_t::simd128:
      return x86_128_simd;
    case composite_t::simd256:
      return x86_256_simd;
    default:
      NOT_IMPLEMENT
  }
}

}  // namespace x86
}  // namespace backends
}  // namespace cinn
