#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include "cinn/utils/macros.h"

namespace cinn {

using float32_t = float;
using float64_t = double;

enum class type_code_t {
  Int,
  Float,
  Boolean,
  Handle,
  UNK,
};

enum class primitive_t : int {
  unk = -1,
  uint8,
  uint16,
  uint32,
  uint64,
  int8,
  int16,
  int32,
  int64,
  float32,
  float64,
  boolean,
  void_,  // control statement without primitive return, such as function, for, if, allocate and so on.
};

enum class composite_t : int {
  primitive = 0,
  simd128,
  simd256,
};

enum class impl_detail_t {
  kNormal = 0,
  kReference = 1,
  kAddress = 2,
};

//! Predifined expression ids used in ir::Identity.
struct expr_ids {
  //! Mark that a expr is used as address.
  static const char* reference_address;
};

static bool IsSimdType(composite_t x) { return x == composite_t::simd128 || x == composite_t::simd256; }
composite_t ToSimdType(int vector_width);

//! Get a string representation of a primitive type.
static std::string ptype_to_str(primitive_t type) {
  switch (type) {
#define __(type__)          \
  case primitive_t::type__: \
    return #type__;

    PRIMITIVE_TYPE_FOR_EACH(__)

    case primitive_t::void_:
      return "void";
    case primitive_t::unk:
      return "unk";

#undef __
  }
}

//! Get a string representation for a primitive_t represented pointer type.
static std::string ptype_to_pointer_type_repr(primitive_t type) { return ptype_to_str(type) + "*"; }

// TODO(Superjomn) compose this to Type.
static bool is_integer(primitive_t ptype) {
  return ptype == primitive_t::int32 || ptype == primitive_t::int8 || ptype == primitive_t::int16 ||
         ptype == primitive_t::int64;
}
static bool is_float(primitive_t ptype) { return ptype == primitive_t::float32 || ptype == primitive_t::float64; }

std::ostream& operator<<(std::ostream& os, primitive_t t);
std::ostream& operator<<(std::ostream& os, composite_t t);
std::ostream& operator<<(std::ostream& os, impl_detail_t t);

enum class memory_location_t {
  host = 0,
  gpu = 1,
};

enum class argument_t {
  unknown = -1,
  input = 0,
  output = 1,
};

//! Get number of bytes of a primitive type.
int primitive_bytes(primitive_t type);

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
