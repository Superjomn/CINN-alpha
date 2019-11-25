# include the generate source file.
include_directories(${CMAKE_BINARY_DIR})

function (_exe_test_ id test_src launcher_src)
  set(oneValueArgs "")
  cmake_parse_arguments(exe_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  cc_test(exe_test${id} SRCS ${test_src} DEPS cinn_lib hlir_lib)
  cc_test(test${id}_c_launcher SRCS ${launcher_src} DEPS cinn_lib hlir_lib)
  add_dependencies(test${id}_c_launcher exe_test${id})
endfunction()

_exe_test_(1 test1.cc test1_c_launcher.cc)
_exe_test_(2 test2.cc test2_c_launcher.cc)
_exe_test_(3 test3.cc test3_c_launcher.cc)

cc_test(exe_test4 SRCS test4.cc DEPS cinn_lib)
cc_test(exe_test5 SRCS test5.cc DEPS cinn_lib)

_exe_test_(6 test6_vectorize.cc test6_launcher.cc)
_exe_test_(7 test7_fc_model.cc test7_launcher.cc)
_exe_test_(8 test8_fc_autotune.cc test8_c_launcher.cc)
_exe_test_(9 test9.cc test9_c_launcher.cc)
_exe_test_(10 test10.cc test10_c_launcher.cc)
_exe_test_(11 test11.cc test11_c_launcher.cc)
