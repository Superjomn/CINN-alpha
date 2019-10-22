cc_library(hlir_pass SRCS graph_to_ir_functions_pass.cc DEPS cinn_lib graph session program op_registry tensor operator)
cc_library(hlir_optimizer SRCS optimizer.cc DEPS graph hlir_pass cinn_lib program graph_util)
cc_test(test_hlir_otimize SRCS hlir_optimize_test.cc DEPS hlir_optimizer)
