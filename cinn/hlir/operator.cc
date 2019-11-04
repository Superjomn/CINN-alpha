#include "cinn/hlir/operator.h"

namespace cinn {
namespace hlir {

void Operator::SetInput(const std::string &argument, const std::string &value) {
  CHECK(!argument.empty());
  CHECK(!value.empty());
  input_argument2value_[argument] = value;
}

const Tensor *Operator::GetInput(const std::string &argument) const {
  CHECK(session_);
  auto it = input_argument2value_.find(argument);
  if (it == input_argument2value_.end()) return nullptr;
  return session_->GetTensor(it->second);
}

Tensor &Operator::GetOutput(const std::string &argument) {
  CHECK(session_);
  auto it = output_argument2value_.find(argument);
  CHECK(it != output_argument2value_.end()) << argument << " not exists in the session";
  return *session_->GetTensor(it->second);
}

void Operator::set_session(Session *x) {
  CHECK(x);
  session_ = x;
}

}  // namespace hlir
}  // namespace cinn
