llvm_map_components_to_libnames(llvm_libs core
        ExecutionEngine
        Interpreter
        MC
        MCJIT
        Support
        nativecodegen
        )
message(STATUS "LLVM libs: ${llvm_libs}")
cc_test(test_llvm_headers SRCS llvm_headers_test.cc DEPS ${llvm_libs})