#pragma once

namespace cinn {

struct Target {
  enum class OS { UNK = 0, Linux = 1 };

  enum class Arch {
    UNK = 0,
    X86,
    ARM,
  };

  enum class Bits {
    UNK = 0,
    k32 = 32,
    k64 = 64,
  };

 private:
  OS os_;
  Arch arch_;
  Bits bits_;

 public:
  Target(OS os = OS::UNK, Arch arch = Arch::X86, Bits bits = Bits::k64) : os_(os), arch_(arch), bits_(bits) {}

  OS os() const { return os_; }
  Arch arch() const { return arch_; }
  Bits bits() const { return bits_; }
};

}  // namespace cinn
