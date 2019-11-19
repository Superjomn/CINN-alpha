cc_library(pass SRCS pass.cc DEPS ir)
cc_library(pass_registry SRCS pass_registry.cc DEPS pass)
cc_library(passes SRCS indices_to_absolute_offset_pass.cc
        simplify_pass.cc
        increment_assign_compose_pass.cc
        fold_reference_indices_pass.cc
        nested_block_clean_pass.cc
        vectorize_pass.cc
        display_program_pass.cc
        DEPS pass pass_registry)

cc_library(optimizer SRCS optimizer.cc DEPS ir passes pass_registry)

cc_test(test_optimizer SRCS optimizer_test.cc DEPS ir optimizer passes)
