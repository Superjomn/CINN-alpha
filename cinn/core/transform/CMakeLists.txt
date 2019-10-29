cc_library(schedule_tree_writer SRCS schedule_tree_writer.cc)
target_link_libraries(schedule_tree_writer ${isl_libs})

cc_library(transforms SRCS transforms.cc DEPS schedule_tree_writer isl_utils logging)

cc_test(test_schedule_tree_writer SRCS schedule_tree_writer_test.cc DEPS schedule_tree_writer)
cc_test(test_transforms SRCS transforms_test.cc DEPS transforms)
