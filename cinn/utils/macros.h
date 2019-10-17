#pragma once

// clang-format off
#define NODETY_PRIMITIVE_TYPE_FOR_EACH(macro__) \
  macro__(IntImm)                               \
  macro__(UInt)                                 \
  macro__(FloatImm)                             \
  macro__(String)                               \

#define NODETY_OP_FOR_EACH(macro__) \
  macro__(Add)                      \
  macro__(Sub)                      \
  macro__(Mul)                      \
  macro__(Div)                      \
  macro__(Mod)                      \
  macro__(Minus)                    \
  macro__(EQ)                       \
  macro__(NE)                       \
  macro__(LT)                       \
  macro__(LE)                       \
  macro__(GT)                       \
  macro__(GE)                       \
  macro__(And)                      \
  macro__(Or)                       \
  macro__(Not)                      \
  macro__(Exp)                      \
  macro__(Assign)                   \
  macro__(Let)                      \
  macro__(SumAssign)                \
  macro__(SubAssign)                \
  macro__(MulAssign)                \
  macro__(DivAssign)                \

#define NODETY_CONTROL_OP_FOR_EACH(macro__) \
  macro__(For)                              \
  macro__(IfThenElse)                       \
  macro__(Block)                            \
  macro__(Call)                             \
  macro__(Function)                         \

#define NODETY_DS_FOR_EACH(macro__) \
  macro__(Var)                      \
  macro__(Param)                    \
  macro__(Tensor)                   \
  macro__(Reference)                \
  macro__(Statement)                \
  macro__(Allocate)                 \
  macro__(Parameter)                \

#define NODETY_MATH_FUNCTION_FOR_EACH(macro__)  \
  macro__(Tanh)                                 \
  macro__(Sigmoid)                              \
  macro__(Max)                                  \
  macro__(Min)

// clang-format on
#define NODETY_FOR_EACH(macro__)          \
  NODETY_PRIMITIVE_TYPE_FOR_EACH(macro__) \
  NODETY_OP_FOR_EACH(macro__)             \
  NODETY_CONTROL_OP_FOR_EACH(macro__)     \
  NODETY_DS_FOR_EACH(macro__)             \
  NODETY_MATH_FUNCTION_FOR_EACH(macro__)

// clang-format off
#define OP_2_ARGS_FOR_EACH(macro__) \
  macro__(Add)    \
  macro__(Sub)    \
  macro__(Mul)    \
  macro__(Div)    \
  macro__(Mod)    \
  macro__(EQ)     \
  macro__(NE)     \
  macro__(LE)     \
  macro__(LT)     \
  macro__(GE)     \
  macro__(GT)     \
  macro__(Min)    \
  macro__(Max)    \
  macro__(And)    \
  macro__(Or)     \
  macro__(Assign) \
  macro__(SumAssign) \
  macro__(SubAssign) \
  macro__(MulAssign) \
  macro__(DivAssign) \
  macro__(Let)    \

#define OP_1_ARGS_FOR_EACH(macro__) \
  macro__(Minus)                    \
  macro__(Exp)                      \
  macro__(Tanh)                     \
  macro__(Sigmoid)

#define IMM_FOR_EACH(macro__) \
  macro__(IntImm)             \
  macro__(FloatImm)

#define OP_ALL_WITHOUT_FUNCTION_FOR_EACH(macro__)       \
  OP_2_ARGS_FOR_EACH(macro__)                           \
  OP_1_ARGS_FOR_EACH(macro__)                           \
  IMM_FOR_EACH(macro__)                                 \
  macro__(Var)                                          \
  macro__(For)                                          \
  macro__(IfThenElse)                                   \
  macro__(Reference)                                    \
  macro__(Block)                                        \
  macro__(Tensor)                                       \
  macro__(Allocate) macro__(Call)                       \
  macro__(Param)

#define OP_ALL_FOR_EACH(macro__)           \
OP_ALL_WITHOUT_FUNCTION_FOR_EACH(macro__)  \
macro__(Function)

// clang-format on

#define NOT_IMPLEMENT LOG(FATAL) << "Not Implemented";

#define PRIMITIVE_TYPE_FOR_EACH(macro__)                                                                     \
  macro__(uint8) macro__(uint16) macro__(uint32) macro__(uint64) macro__(int8) macro__(int16) macro__(int32) \
      macro__(int64) macro__(float32) macro__(float64) macro__(boolean)
