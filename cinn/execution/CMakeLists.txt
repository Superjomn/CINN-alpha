cc_library(simd SRCS simd.cc)

cc_library(cinn_execution SRCS cinn_execution.cc DEPS simd)

cc_test(test_simd SRCS simd_test.cc DEPS cinn_execution)
