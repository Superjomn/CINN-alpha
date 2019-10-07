cc_library(any SRCS any.cc)
cc_library(name_generator SRCS name_generator.cc)
cc_library(isl_utils SRCS isl_utils.cc)
cc_library(logging SRCS logging.cc)
cc_library(math SRCS math.cc)
target_link_libraries(isl_utils ${isl_lib})

cc_library(utils DEPS any name_generator isl_utils logging math)

cc_test(test_isl_utils SRCS isl_utils_test.cc DEPS isl_utils)
target_link_libraries(test_isl_utils ${isl_lib})
