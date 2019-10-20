set(op_deps operator op_registry cinn_lib)
cc_library(activation_op SRCS activation_op.cc DEPS ${op_deps})
cc_library(pad_op SRCS pad_op.cc DEPS ${op_deps})

cc_test(test_pad_op SRCS pad_op_test.cc DEPS pad_op cinn_lib)
