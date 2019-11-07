#cc_library(schedule_tree SRCS schedule_tree.cc DEPS ir)

add_subdirectory(transform)
add_subdirectory(optimize)

cc_library(stage SRCS stage.cc DEPS ir isl_utils isl_code_gen)
#cc_library(buffer SRCS buffer.cc DEPS ir)
cc_library(isl_code_gen SRCS isl_code_gen.cc DEPS ir)
cc_library(function SRCS function.cc DEPS ir stage isl_code_gen transforms)
cc_test(test_function SRCS function_test.cc DEPS function ir ops_overload)
cc_library(schedule_tree SRCS schedule_tree.cc DEPS isl_code_gen)
cc_library(schedule_tree_util SRCS schedule_tree_util.cc DEPS schedule_tree)

cc_test(test_stage SRCS stage_test.cc DEPS function stage isl_utils)
cc_test(test_buffer SRCS buffer_test.cc DEPS function stage isl_utils)
#cc_test(test_isl_code_gen SRCS isl_code_gen_test.cc DEPS isl_code_gen ir_printer stage function)
target_link_libraries(test_stage ${isl_lib})
target_link_libraries(test_function ${isl_lib})
