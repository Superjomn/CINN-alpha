#include "cinn/type.h"
#include <glog/logging.h>

namespace cinn {

int primitive_bytes(primitive_t type) {
  switch (type) {
    case primitive_t::uint8:
    case primitive_t::int8:
      return 1;
    case primitive_t::int16:
    case primitive_t::uint16:
      return 2;
    case primitive_t::int32:
    case primitive_t::uint32:
    case primitive_t::float32:
      return 4;
    case primitive_t::int64:
    case primitive_t::uint64:
    case primitive_t::float64:
      return 8;
    case primitive_t::boolean:
      return 1;
    case primitive_t::unk:
      return -1;
  }
}

std::ostream &operator<<(std::ostream &os, primitive_t t) {
  switch (t) {
#define __(type)          \
  case primitive_t::type: \
    os << #type;          \
    break;
    __(unk);
    __(uint8);
    __(uint16);
    __(uint32);
    __(uint64);
    __(int8);
    __(int16);
    __(int32);
    __(int64);
    __(float32);
    __(float64);
    __(boolean);
#undef __
    default:
      LOG(FATAL) << "unsupported primitive type";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, composite_t t) {
  switch (t) {
#define __(type)          \
  case composite_t::type: \
    os << #type;          \
    break;

    __(primitive)
    __(simd128)
    __(simd256)
#undef __
  }
}

composite_t ToSimdType(int vector_width) {
  switch (vector_width) {
    case 4:
      return composite_t::simd128;
    case 8:
      return composite_t::simd256;
    default:
      NOT_IMPLEMENT
  }
}

std::ostream &operator<<(std::ostream &os, impl_detail_t t) {
  switch (t) {
    case impl_detail_t::kNormal:
      os << "normal";
      break;
    case impl_detail_t::kAddress:
      os << "address";
      break;
    case impl_detail_t::kReference:
      os << "reference";
      break;
  }
}

const char *expr_ids::reference_address = "reference_address";

}  // namespace cinn
