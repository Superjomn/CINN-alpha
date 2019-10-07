cc_test(exe_test1 SRCS test1.cc DEPS cinn_lib)

# include the generate source file.
include_directories(${CMAKE_BINARY_DIR})

cc_library(test1_exec SRCS ${CMAKE_BINARY_DIR}/exe_tests/exe_test1.cc)
cc_test(test1_C_launcher SRCS test1_C_launcher.cc DEPS test1_exec utils)
add_dependencies(test1_C_launcher test1_exec)
