cc_library(python_api SRCS python_api.cc DEPS pybind cinn_lib hlir_lib SHARED)
set_target_properties(python_api PROPERTIES OUTPUT_NAME "cinn_core"
  PREFIX ""
  )
