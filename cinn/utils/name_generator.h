#pragma once
#include <cstddef>
#include <string>

namespace cinn {

class NameGenerator {
 private:
  static size_t func_counter_;
  static size_t stage_counter_;
  static size_t iterator_counter_;
  static size_t parameter_counter_;
  static size_t var_counter_;
  static size_t named_counter_;

 public:
  static NameGenerator& Global() {
    static NameGenerator x;
    return x;
  }

  std::string NewFuncionName() const { return "func" + std::to_string(func_counter_++); }
  std::string NewStageName() const { return "S" + std::to_string(func_counter_++); }
  std::string NewIteratorName() const { return "i" + std::to_string(func_counter_++); }
  std::string NewParameterName() const { return "p" + std::to_string(parameter_counter_++); }
  std::string NewVarName() const { return "var" + std::to_string(var_counter_++); }
  std::string NewNamed(const std::string& x) { return x + std::to_string(named_counter_++); }

 private:
  NameGenerator() = default;
};

}  // namespace cinn
