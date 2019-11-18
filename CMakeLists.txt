cmake_minimum_required(VERSION 3.0)

project(CINN)

set(CMAKE_CXX_STANDARD 11)

option(WITH_TESTING         "Compile with Unittests"          ON)

include(cmake/core.cmake)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

# Include third-party libraries.
set(THIRD_PARTY_PATH ${CMAKE_BINARY_DIR}/third_party)
include(cmake/external/pybind11.cmake)
include(cmake/external/gflags.cmake)
include(cmake/external/glog.cmake)
include(cmake/external/gtest.cmake)
include(cmake/external/protobuf.cmake)

add_subdirectory(cinn)
add_subdirectory(exe_tests)
