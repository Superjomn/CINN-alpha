cc_library(expr SRCS expr.cc DEPS glog)
cc_library(ir SRCS ir.cc ir_helper.cc ir_visitor.cc ir_mutator.cc ir_printer.cc ir_mutator_helpers.cc DEPS expr ir_node_base any name_generator logging isl_utils type)
cc_library(ir_node_base SRCS node_base.cc)
cc_library(ops_overload SRCS ops_overload.cc DEPS ir)

cc_test(test_ir_printer SRCS ir_printer_test.cc DEPS ir stage)
cc_test(test_ir_visitor SRCS ir_visitor_test.cc DEPS ir)
cc_test(test_ir SRCS ir_test.cc DEPS ir ops_overload)
cc_test(test_ir_helper SRCS ir_helper_test.cc DEPS ir ops_overload stage)
