cmake_minimum_required(VERSION 3.0)

add_definitions(-std=c++11)

include(cmake/core.cmake)

include_directories(${CMAKE_SOURCE_DIR})

# Include third-party libraries.
set(THIRD_PARTY_PATH ${CMAKE_BINARY_DIR}/third_party)
include(cmake/external/gflags.cmake)
include(cmake/external/glog.cmake)
include(cmake/external/gtest.cmake)

add_subdirectory(cinn)
