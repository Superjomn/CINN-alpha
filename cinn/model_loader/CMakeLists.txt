add_subdirectory(fluid)

cc_library(ml_tensor SRCS tensor.cc)
cc_library(ml_host_buffer SRCS host_buffer.cc)
cc_library(ml_scope SRCS scope.cc)
cc_library(ml_variable SRCS variable.cc)
