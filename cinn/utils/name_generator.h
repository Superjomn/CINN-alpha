#pragma once
#include <cstddef>
#include <string>

namespace cinn {

class NameGenerator {
 public:
  static NameGenerator& Global();

  void ResetCounter();

  std::string NewNamed(const std::string& x) { return x + "_" + std::to_string(named_counter_++); }
  std::string NewFuncionName() { return "func" + std::to_string(func_counter_++); }
  std::string NewStageName() { return "S" + std::to_string(func_counter_++); }
  std::string NewIteratorName() { return "i" + std::to_string(func_counter_++); }
  std::string NewParameterName() { return "p" + std::to_string(parameter_counter_++); }
  std::string NewVarName() { return "var" + std::to_string(var_counter_++); }
  std::string NewBuffer() { return "buf" + std::to_string(buffer_counter_++); }
  std::string NewArray() { return "arr" + std::to_string(array_counter_++); }
  std::string NewTmpVar() { return "tmp" + std::to_string(tmp_var_counter_++); }

 private:
  size_t func_counter_{};
  size_t stage_counter_{};
  size_t iterator_counter_{};
  size_t parameter_counter_{};
  size_t var_counter_{};
  size_t named_counter_{};
  size_t buffer_counter_{};
  size_t array_counter_{};
  size_t tmp_var_counter_{};

  NameGenerator() = default;
  NameGenerator(const NameGenerator&) = delete;
};

}  // namespace cinn
