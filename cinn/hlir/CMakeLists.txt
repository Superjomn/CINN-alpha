cc_library(hlir_util SRCS hlir_util.cc)
cc_library(operator SRCS operator.cc DEPS hlir_util)
cc_library(tensor SRCS tensor.cc DEPS hlir_util)
cc_library(placeholder SRCS placeholder.cc DEPS hlir_util)
cc_library(variable SRCS variable.cc DEPS hlir_util)
cc_library(session SRCS session.cc DEPS hlir_util)
cc_library(op_registry SRCS op_registry.cc)
cc_library(buffer SRCS buffer.cc)
cc_library(network SRCS network.cc DEPS hlir_util program)

set(instruction_ops CACHE INTERNAL "instruction ops")
add_subdirectory(instruction_layer)

cc_library(program SRCS program.cc DEPS hlir_util ${instruction_ops})
cc_library(graph SRCS graph.cc DEPS hlir_util program)
cc_library(graph_util SRCS graph_util.cc DEPS graph)

add_subdirectory(optimize)


message(STATUS "instruction_ops: ${instruction_ops}")
cc_test(test_graph SRCS graph_test.cc DEPS ${instruction_ops} graph session op_registry operator cinn_lib graph_util)
cc_test(test_graph_util SRCS graph_util_test.cc DEPS graph_util cinn_lib session op_registry ${instruction_ops})
cc_test(test_network SRCS network_test.cc DEPS graph_util cinn_lib session op_registry ${instruction_ops} network graph)

cc_library(hlir_lib SRCS hlir.cc DEPS operator tensor graph network graph_util ${instruction_ops})
