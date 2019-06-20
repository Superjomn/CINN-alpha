#set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -pthread -ldl")

cc_library(cinn_gtest_main SRCS gtest_main.cc DEPS gtest gflags glog)
target_link_libraries(cinn_gtest_main "-pthread -ldl")

cc_library(type SRCS type.cc)

add_subdirectory(utils)
add_subdirectory(ir)
