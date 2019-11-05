cc_library(evaluator SRCS evaluator.cc DEPS cinn_lib hlir_lib)
cc_library(point SRCS point.cc DEPS cinn_lib hlir_lib)
cc_test(test_point SRCS point_test.cc DEPS point)
