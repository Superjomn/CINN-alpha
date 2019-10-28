include_directories(${LLVM_INCLUDE_DIR})

llvm_map_components_to_libnames(llvm_libs
        Core
        ExecutionEngine
        Interpreter
        MC
        MCJIT
        Support
        nativecodegen
        native
        )
message(STATUS "LLVM libs: ${llvm_libs}")
cc_library(code_gen_llvm SRCS code_gen_llvm.cc DEPS ir ${llvm_libs})
cc_library(llvm_jit SRCS llvm_jit.cc DEPS ${llvm_libs})

cc_test(test_llvm_headers_ SRCS llvm_headers_test.cc DEPS ${llvm_libs})
cc_test(test_code_gen_llvm SRCS code_gen_llvm_test.cc DEPS code_gen_llvm llvm_jit)
target_link_libraries(test_code_gen_llvm ir function)

cc_test(test_isl SRCS isl_test.cc DEPS isl_utils)
target_link_libraries(test_isl ${isl_lib})
cc_test(test_isl_aff SRCS isl_aff_test.cc DEPS isl_utils)

cc_library(code_gen_c SRCS code_gen_c.cc DEPS ir optimizer)
cc_test(test_code_gen_c SRCS code_gen_c_test.cc DEPS code_gen_c function )

cc_test(test_llvm_jit SRCS llvm_jit_test.cc DEPS llvm_jit)
