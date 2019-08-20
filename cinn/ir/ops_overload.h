#pragma once
#include "cinn/ir/ir.h"

/*
 * This file defines various operator overload the utility operators such as '+' '-' '*' '/' to make it easier to use
 * IR.
 */

namespace cinn {
namespace ir {

inline Expr operator+(Expr a, Expr b) { return Add::make(a, b); }
template <typename T>
inline Expr operator+(Expr a, T b) {
  return Add::make(a, Expr(b));
}
template <typename T>
inline Expr operator-(Expr a, T b) {
  return Sub::make(a, Expr(b));
}
template <typename T>
inline Expr operator*(Expr a, T b) {
  return Mul::make(a, Expr(b));
}
template <typename T>
inline Expr operator/(Expr a, T b) {
  return Div::make(a, Expr(b));
}
template <typename T>
inline Expr operator%(Expr a, T b) {
  return Mod::make(a, Expr(b));
}

template <typename T>
inline Expr operator<(Expr a, T b) {
  return LT::make(a, Expr(b));
}
template <typename T>
inline Expr operator<=(Expr a, T b) {
  return LE::make(a, Expr(b));
}
template <typename T>
inline Expr operator>(Expr a, T b) {
  return GT::make(a, Expr(b));
}
template <typename T>
inline Expr operator>=(Expr a, T b) {
  return GE::make(a, Expr(b));
}
template <typename T>
inline Expr operator==(Expr a, T b) {
  return EQ::make(a, Expr(b));
}

inline Expr operator-(Expr a, Expr b) { return Sub::make(a, b); }
inline Expr operator*(Expr a, Expr b) { return Mul::make(a, b); }
inline Expr operator/(Expr a, Expr b) { return Div::make(a, b); }
inline Expr operator%(Expr a, Expr b) { return Mod::make(a, b); }

}  // namespace ir
}  // namespace cinn
