namespace cinn {

struct Target {
  enum class OS {
    UNK = 0,
    linux,
  };

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
  Target() : os_(OS::UNK), arch_(Arch::UNK), bits_(Bits::UNK) {}

  OS os() const { return os_; }
  Arch arch() const { return arch_; }
  Bits bits() const { return bits_; }
};

}  // namespace cinn
