set(op_deps operator op_registry cinn_lib tensor)

cc_library(activation_op SRCS activation_op.cc DEPS ${op_deps})
cc_library(pad_op SRCS pad_op.cc DEPS ${op_deps})
cc_library(reshape_op SRCS reshape_op.cc DEPS ${op_deps})
cc_library(matmul_op SRCS matmul_op.cc DEPS ${op_deps})


set(ops activation_op pad_op reshape_op matmul_op)

cc_test(test_pad_op SRCS pad_op_test.cc DEPS ${ops} cinn_lib)
cc_test(test_matmul_op SRCS matmul_op_test.cc DEPS ${ops} cinn_lib)
