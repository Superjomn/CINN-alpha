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
      return 4;
    case primitive_t::int64:
    case primitive_t::uint64:
      return 8;
    case primitive_t::boolean:
      return 1;
    default:
      LOG(FATAL) << "unsupported type " << static_cast<int>(type);
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
#undef __
  }
}

}  // namespace cinn
