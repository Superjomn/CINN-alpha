proto_library(framework_proto SRCS framework.proto)

cc_library(var_desc SRCS var_desc.cc DEPS framework_proto)
cc_library(program_desc SRCS program_desc.cc DEPS framework_proto)
cc_library(op_desc SRCS op_desc.cc DEPS framework_proto)
cc_library(block_desc SRCS block_desc.cc DEPS framework_proto)
cc_library(parse_model SRCS parse_model.cc DEPS var_desc program_desc op_desc block_desc)
