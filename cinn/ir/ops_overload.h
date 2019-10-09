#pragma once
#include "cinn/ir/ir.h"

/*
 * This file defines various operator overload the utility operators such as '+' '-' '*' '/' to make it easier to use
 * IR.
 */

namespace cinn {
namespace ir {

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
  LOG(INFO) << "get a mod%";
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
template <typename T>
inline Expr operator!=(Expr a, T b) {
  return NE::make(a, Expr(b));
}

}  // namespace ir

ir::Expr Tanh(const ir::Expr &e);
ir::Expr Exp(const ir::Expr &e);
ir::Expr Sigmoid(const ir::Expr &e);
ir::Expr Max(const ir::Expr &a, const ir::Expr &b);
ir::Expr Min(const ir::Expr &a, const ir::Expr &b);

}  // namespace cinn
