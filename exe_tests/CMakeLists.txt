cc_test(exe_test1 SRCS test1.cc DEPS cinn_lib)

# include the generate source file.
include_directories(${CMAKE_BINARY_DIR})

cc_test(test1_c_launcher SRCS test1_c_launcher.cc)
add_dependencies(test1_c_launcher exe_test1)

cc_test(exe_test2 SRCS test2.cc DEPS cinn_lib)
cc_test(test2_c_launcher SRCS test2_c_launcher.cc)
