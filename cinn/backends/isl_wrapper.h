#include <glog/logging.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/space.h>

namespace isl {

struct Context {
  isl_ctx* ctx{};
  bool own{false};

  Context() : ctx(isl_ctx_alloc()), own{true} {}
  Context(isl_ctx* x) : ctx{x} {}

  void set_max_operations(unsigned long x) { isl_ctx_set_max_operations(ctx, x); }
  unsigned long max_operations() const { return isl_ctx_get_max_operations(ctx); }
  void reset_max_operations() { isl_ctx_reset_operations(ctx); }

  ~Context() {
    CHECK(ctx);
    if (own) {
      isl_ctx_free(ctx);
    }
  }
};

struct Value {
  using value_type = isl_val*;
  value_type val{};
  isl_ctx* ctx{};

  Value() = default;
  Value(isl_ctx* ctx) : ctx(ctx) {}
  Value(value_type x) : val(x) {}
  ~Value() { isl_val_free(val); }

  void copy_from(const Value& other) {
    CHECK(!val);
    val = isl_val_copy(other.val);
  }

  void as_one() {
    CHECK(!val);
    CHECK(ctx);
    val = isl_val_one(ctx);
  }

  void as_zero() {
    CHECK(!val);
    CHECK(ctx);
    val = isl_val_zero(ctx);
  }

  bool is_zero() const { return isl_val_is_zero(val); }
  bool is_one() const { return isl_val_is_one(val); }

  friend bool operator<(const Value& a, const Value& b) { return isl_val_lt(a.val, b.val); }
  friend bool operator>(const Value& a, const Value& b) { return isl_val_gt(a.val, b.val); }
  friend bool operator<=(const Value& a, const Value& b) { return isl_val_le(a.val, b.val); }
  friend bool operator>=(const Value& a, const Value& b) { return isl_val_ge(a.val, b.val); }
  friend bool operator==(const Value& a, const Value& b) { return isl_val_eq(a.val, b.val); }
  friend bool operator!=(const Value& a, const Value& b) { return isl_val_ne(a.val, b.val); }

  std::string str() const { return isl_val_to_str(val); }
};

struct Id {
  isl_id* id{};

  Id(isl_id* id) : id(id) {}
  ~Id() { isl_id_free(id); }

  void copy_from(const Id& other) {
    CHECK(!id);
    id = isl_id_copy(other.id);
  }

  std::string str() cont { return isl_id_to_str(id); }
};

struct Space {
  isl_space* space;

  Space(isl_space* space) : space(space) {}
  ~Space() { isl_space_free(space); }

  bool is_params() const { return isl_space_is_params(space); }
  bool is_set() const { return isl_space_is_set(space); }
  bool is_map() const { return isl_space_is_map(space); }

  std::string str() const { return isl_space_to_str(space); }
};

struct Set {
  using value_type = isl_set*;
  value_type set;

  Set(value_type x) : set(x) {}
  Set(Context* ctx, const std::string& x) { set = isl_set_read_from_str(ctx->ctx, x.c_str()); }
  Set(Space* space) : set(isl_set_empty(space->space)) {}
  isl_space* get_space() { return isl_set_get_space(set); }
  std::string str() const { return isl_set_to_str(set); }
};

}  // namespace isl
