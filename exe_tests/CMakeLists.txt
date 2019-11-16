cc_test(exe_test1 SRCS test1.cc DEPS cinn_lib)

# include the generate source file.
include_directories(${CMAKE_BINARY_DIR})

cc_test(test1_c_launcher SRCS test1_c_launcher.cc)
add_dependencies(test1_c_launcher exe_test1)

cc_test(exe_test2 SRCS test2.cc DEPS cinn_lib)
cc_test(test2_c_launcher SRCS test2_c_launcher.cc)

cc_test(exe_test3 SRCS test3.cc DEPS cinn_lib)
cc_test(test3_c_launcher SRCS test3_c_launcher.cc)

cc_test(exe_test4 SRCS test4.cc DEPS cinn_lib)
cc_test(exe_test5 SRCS test5.cc DEPS cinn_lib)

cc_test(exe_test6 SRCS test6_vectorize.cc DEPS cinn_lib)
cc_test(test6_c_launcher SRCS test6_launcher.cc DEPS cinn_lib)

cc_test(exe_test7 SRCS test7_fc_model.cc DEPS cinn_lib hlir_lib)
cc_test(test7_c_launcher SRCS test7_launcher.cc DEPS cinn_lib)

add_dependencies(test6_c_launcher exe_test6)
add_dependencies(test7_c_launcher exe_test7)
