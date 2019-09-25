llvm_map_components_to_libnames(llvm_libs core
        ExecutionEngine
        Interpreter
        MC
        MCJIT
        Support
        nativecodegen
        )
message(STATUS "LLVM libs: ${llvm_libs}")
#cc_test(test_llvm_headers SRCS llvm_headers_test.cc DEPS ${llvm_libs})
cc_test(test_isl SRCS isl_test.cc)
target_link_libraries(test_isl ${isl_lib})

cc_library(code_gen_c SRCS code_gen_c.cc DEPS ir_printer ir optimizer)
cc_test(test_code_gen_c SRCS code_gen_c_test.cc DEPS code_gen_c function)
