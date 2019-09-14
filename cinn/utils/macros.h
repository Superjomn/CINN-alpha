#pragma once

#define OP_2_ARGS_FOR_EACH(macro__)                                                                                \
  macro__(Add) macro__(Sub) macro__(Mul) macro__(Div) macro__(Mod) macro__(EQ) macro__(NE) macro__(LE) macro__(LT) \
      macro__(GE) macro__(GT) macro__(Min) macro__(Max) macro__(And) macro__(Or) macro__(Assign)

#define OP_1_ARGS_FOR_EACH(macro__) macro__(Minus);

#define IMM_FOR_EACH(macro__) macro__(IntImm) macro__(FloatImm)

#define OP_ALL_FOR_EACH(macro__)                                                                                  \
  OP_2_ARGS_FOR_EACH(macro__)                                                                                     \
  OP_1_ARGS_FOR_EACH(macro__)                                                                                     \
  IMM_FOR_EACH(macro__)                                                                                           \
  macro__(Var) macro__(For) macro__(IfThenElse) macro__(Reference) macro__(Block) macro__(Allocate) macro__(Call) \
      macro__(Function) macro__(Param)

#define PRIMITIVE_TYPE_FOR_EACH(macro__)                                                                     \
  macro__(uint8) macro__(uint16) macro__(uint32) macro__(uint64) macro__(int8) macro__(int16) macro__(int32) \
      macro__(int64) macro__(float32) macro__(float64) macro__(boolean)
