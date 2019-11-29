#include "cinn/backends/code_gen_c.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "cinn/core/function.h"
#include "cinn/core/isl_code_gen.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {

using cs = std::vector<ir::Constant>;

TEST(code_gen_c, easy) {
  SetGlobalContext(new CINNContext);

  Function fn("fn0");
  {
    ir::Constant M("M", 20);
    ir::Var i("i");

    ir::Expr A({M}, primitive_t::float32, "A");
    fn.AddStage(A[i].Assign(ir::Expr(0.f)));

    fn.Inputs({A});
    fn.Outputs({A});
    fn.EndDefinition();
  }

  backends::C_CodeGen code_gen;
  code_gen(Expr(fn));

  std::string log = code_gen.compiled_code();
  LOG(INFO) << "generated code: \n" << log;

  std::string target = R"ROC(#ifndef CINN_FILE_
#define CINN_FILE_
#include <immintrin.h>
#include <math.h>
#include <simd.h>
#include <stdio.h>

typedef bool cinn_boolean_t;
typedef char cinn_int8_t;
typedef int cinn_int32_t;
typedef long long cinn_int64_t;
typedef unsigned char cinn_uint8_t;
typedef unsigned int cinn_uint32_t;
typedef unsigned long long cinn_uint64_t;
typedef float cinn_float32_t;

#define cinn_min(a,b) ((a)<(b) ? (a) : (b))
#define cinn_max(a,b) ((a)>(b) ? (a) : (b))
#define cinn_copy(a,b,size) memcpy((b), (a), (size))


void fn0 (cinn_float32_t* A, cinn_float32_t* A) {
  for (int c0 = 0; (c0 <= 19); c0 += 1) {
    A[c0] = 0;
  }
}

#endif  // CINN_FILE_
)ROC";

  EXPECT_EQ(log, target);
}

TEST(cpp_code_gen, basic) {
  SetGlobalContext(new CINNContext);

  ir::Constant M(100), N(200), K(300);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  ir::Var i("i"), j("j"), k("k");

  Function fn("fn");
  {
    Stage s0 = fn.AddStage(B[i + 1][j].Assign(  //
        (A[i - 1][j] + A[i][j] + A[i + 1][j]) / 3.f));

    Stage s1 = fn.AddStage(C[i][j].Assign(  //
        A[i][j] * 2.f + B[i][j] / 2.f));

    fn.Inputs({A, B});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  {
    backends::C_CodeGen code_gen;
    code_gen(Expr(fn));

    std::string log = code_gen.compiled_code();
    LOG(INFO) << "generated code: \n" << log;

    std::string target =
        R"ROC(#ifndef CINN_FILE_
#define CINN_FILE_
#include <immintrin.h>
#include <math.h>
#include <simd.h>
#include <stdio.h>

typedef bool cinn_boolean_t;
typedef char cinn_int8_t;
typedef int cinn_int32_t;
typedef long long cinn_int64_t;
typedef unsigned char cinn_uint8_t;
typedef unsigned int cinn_uint32_t;
typedef unsigned long long cinn_uint64_t;
typedef float cinn_float32_t;

#define cinn_min(a,b) ((a)<(b) ? (a) : (b))
#define cinn_max(a,b) ((a)>(b) ? (a) : (b))
#define cinn_copy(a,b,size) memcpy((b), (a), (size))


void fn (cinn_float32_t* A, cinn_float32_t* B, cinn_float32_t* C) {
  for (int c0 = 1; (c0 <= 98); c0 += 1) {
    for (int c1 = 0; (c1 <= 199); c1 += 1) {
      B[(((c0 + 1) * 200) + c1)] = (((A[(((c0 - 1) * 300) + c1)] + A[((c0 * 300) + c1)]) + A[(((c0 + 1) * 300) + c1)]) / 3);
    }
  }
  for (int c0 = 0; (c0 <= 99); c0 += 1) {
    for (int c1 = 0; (c1 <= 199); c1 += 1) {
      C[((c0 * 200) + c1)] = ((A[((c0 * 300) + c1)] * 2) + (B[((c0 * 200) + c1)] / 2));
    }
  }
}

#endif  // CINN_FILE_
)ROC";

    LOG(INFO) << "gen code:\n" << log;
    ASSERT_EQ(log, target);
  }

  {
    backends::C_CodeGen code_gen(false);
    code_gen(Expr(fn));

    std::string log = code_gen.compiled_code();

    LOG(INFO) << "header:\n" << log;

    std::string target = R"ROC(#ifndef CINN_FILE_
#define CINN_FILE_
#include <immintrin.h>
#include <math.h>
#include <simd.h>
#include <stdio.h>

typedef bool cinn_boolean_t;
typedef char cinn_int8_t;
typedef int cinn_int32_t;
typedef long long cinn_int64_t;
typedef unsigned char cinn_uint8_t;
typedef unsigned int cinn_uint32_t;
typedef unsigned long long cinn_uint64_t;
typedef float cinn_float32_t;

#define cinn_min(a,b) ((a)<(b) ? (a) : (b))
#define cinn_max(a,b) ((a)>(b) ? (a) : (b))
#define cinn_copy(a,b,size) memcpy((b), (a), (size))


void fn (cinn_float32_t* A, cinn_float32_t* B, cinn_float32_t* C);


#endif  // CINN_FILE_
)ROC";

    ASSERT_EQ(log, target);
  }
}

TEST(cpp_code_gen, mat_mul) {
  SetGlobalContext(new CINNContext);

  ir::Constant M(100), N(200), K(300);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  ir::Var i("i"), j("j"), k("k");

  Function fn("fn");
  {
    Stage s0 = fn.AddStage(C[i][j] = A[i][k] * B[k][j]);

    fn.Inputs({A, B});
    fn.Outputs({C});

    fn.EndDefinition();
  }

  {
    backends::C_CodeGen code_gen;
    code_gen(Expr(fn));

    std::string log = code_gen.compiled_code();
    LOG(INFO) << "generated code: \n" << log;
  }
}

namespace backends {}  // namespace backends
}  // namespace cinn
