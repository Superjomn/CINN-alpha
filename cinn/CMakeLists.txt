#=================================== ISL =======================================
set(ISL_LIB_DIRECTORY "/usr/local")
#set(ISL_LIB_DIRECTORY "/Users/yanchunwei/project/tiramisu/3rdParty/isl/build")
find_library(isl_lib isl PATHS "${ISL_LIB_DIRECTORY}/lib" NO_DEFAULT_PATH)
include_directories(${ISL_LIB_DIRECTORY}/include)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/isl_interface/include)

link_directories(${ISL_LIB_DIRECTORY}/lib)
message(STATUS "isl path: ${ISL_LIB_DIRECTORY}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
set(LLVM_VERSION "${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR}")

file(TO_NATIVE_PATH "${LLVM_TOOLS_BINARY_DIR}/llvm-as${CMAKE_EXECUTABLE_SUFFIX}" LLVM_AS)
file(TO_NATIVE_PATH "${LLVM_TOOLS_BINARY_DIR}/llvm-nm${CMAKE_EXECUTABLE_SUFFIX}" LLVM_NM)
file(TO_NATIVE_PATH "${LLVM_TOOLS_BINARY_DIR}/clang${CMAKE_EXECUTABLE_SUFFIX}" CLANG)
file(TO_NATIVE_PATH "${LLVM_TOOLS_BINARY_DIR}/llvm-config${CMAKE_EXECUTABLE_SUFFIX}" LLVM_CONFIG)

# LLVM doesn't appear to expose --system-libs via its CMake interface,
# so we must shell out to llvm-config to find this info
execute_process(COMMAND ${LLVM_CONFIG} --system-libs --link-static OUTPUT_VARIABLE CINN_SYSTEM_LIBS_RAW)
execute_process(COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR)
message(STATUS "include LLVM headers ${LLVM_INCLUDE_DIR}")
include_directories(${LLVM_INCLUDE_DIR})

string(STRIP "${CINN_SYSTEM_LIBS_RAW}" CINN_SYSTEM_LIBS_RAW)  # strip whitespace from start & end
string(REPLACE " " ";" CINN_SYSTEM_LIBS "${CINN_SYSTEM_LIBS_RAW}")  # convert into a list

if("${CINN_SYSTEM_LIBS}" STREQUAL "")
  # It's theoretically possible that this could be legitimately empty,
  # but in practice that doesn't really happen, so we'll assume it means we
  # aren't configured correctly.
  message(WARNING "'llvm-config --system-libs --link-static' is empty; this is possibly wrong.")
endif()


cc_library(cinn_gtest_main SRCS gtest_main.cc DEPS gtest gflags glog)
target_link_libraries(cinn_gtest_main "-pthread -ldl")

cc_library(type SRCS type.cc)
cc_library(target SRCS target.cc)
cc_library(module SRCS module.cc)

add_subdirectory(backends)
add_subdirectory(utils)
add_subdirectory(ir)
add_subdirectory(core)

cc_library(cinn_lib SRCS cinn.cc DEPS ir ir_printer code_gen_c code_gen_llvm ${llvm_libs} ops_overload)
