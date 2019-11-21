set(op_deps operator op_registry cinn_lib tensor session)


cc_library(activation_op SRCS activation_op.cc DEPS ${op_deps})
cc_library(pad_op SRCS pad_op.cc DEPS ${op_deps})
cc_library(reshape_op SRCS reshape_op.cc DEPS ${op_deps})
cc_library(matmul_op SRCS matmul_op.cc DEPS ${op_deps})
#cc_library(conv2d_op SRCS conv2d_op.cc DEPS ${op_deps})
cc_library(elementwise_ops SRCS elementwise_ops.cc DEPS ${op_deps})
cc_library(transpose_op SRCS transpose_op.cc DEPS ${op_deps})


set(instruction_ops activation_op pad_op reshape_op matmul_op
        elementwise_ops
        transpose_op
        CACHE INTERNAL "ops")

cc_test(test_pad_op SRCS pad_op_test.cc DEPS ${instruction_ops} cinn_lib)
cc_test(test_matmul_op SRCS matmul_op_test.cc DEPS ${instruction_ops} cinn_lib)
cc_test(test_transpose_op SRCS transpose_op_test.cc DEPS ${instruction_ops} cinn_lib)
