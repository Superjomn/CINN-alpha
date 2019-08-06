cc_library(stage SRCS stage.cc)
cc_library(schedule_tree SRCS schedule_tree.cc DEPS ir)
cc_library(computation SRCS computation.cc DEPS ir)
cc_test(test_computation SRCS computation_test.cc DEPS computation ${isl_lib})
