#pragma once

#define OP_2_ARGS_FOR_EACH(macro__) \
  macro__(Add);                     \
  macro__(Sub);                     \
  macro__(Mul);                     \
  macro__(Div);                     \
  macro__(Mod);                     \
  macro__(EQ);                      \
  macro__(NE);                      \
  macro__(LE);                      \
  macro__(LT);                      \
  macro__(GE);                      \
  macro__(GT);                      \
  macro__(Min);                     \
  macro__(Max);                     \
  macro__(And);                     \
  macro__(Or);

#define OP_1_ARGS_FOR_EACH(macro__) macro__(Minus);
