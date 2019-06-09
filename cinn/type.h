#pragma once

#include <cstdint>

namespace cinn {

enum class type_code_t {
  Int32,
  Int64,
  Fp32,
};

class Type {
 public:
  Type(type_code_t code, uint32_t bits, uint32_t lanes) : type_code_(code), bits_(bits), lanes_(lanes) {}

  bool is_int() const { return type_code_ == type_code_t::Int32; }
  bool is_float() const { return type_code_ == type_code_t::Fp32; }

 private:
  type_code_t type_code_;
  uint32_t bits_{};
  uint32_t lanes_{};
};

}  // namespace cinn
