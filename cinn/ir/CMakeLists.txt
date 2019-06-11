cc_library(expr SRCS expr.cc DEPS glog)
cc_library(ir SRCS ir.cc DEPS expr)
cc_library(ir_visitor SRCS ir_visitor)
