#pragma once

#include <cstdint>

namespace cinn {

enum class type_code_t {
  Int,
  Float,
  Handle,
  UNK,
};

class Type {
 public:
  Type() = default;
  explicit Type(type_code_t code) : type_code_(code) {}
  Type(type_code_t code, uint32_t bits, uint32_t lanes = 1) : type_code_(code), bits_(bits), lanes_(lanes) {}

  bool is_int() const { return type_code_ == type_code_t::Int; }
  bool is_float() const { return type_code_ == type_code_t::Float; }

  uint32_t bits() const { return bits_; }
  uint32_t lanes() const { return lanes_; }

 private:
  type_code_t type_code_{type_code_t::UNK};
  uint32_t bits_{};
  uint32_t lanes_{1};
};

}  // namespace cinn
