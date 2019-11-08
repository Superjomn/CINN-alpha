#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "cinn/model_loader/fluid/parse_model.h"
#include "cinn/model_loader/fluid/program_desc.h"
#include "cinn/model_loader/scope.h"
#include "cinn/model_loader/variable.h"

namespace cinn {
namespace model_loader {
namespace fluid {

// Read a __model__ file.
std::unique_ptr<framework::proto::ProgramDesc> LoadProgram(const std::string& path, bool program_from_memory = false);

// Read a single file containing all the parameters.
void LoadParams(const std::string& path);

// Load a single parameter to an output tensor.
void LoadParam(const std::string& path, Variable* out);

void LoadCombinedParamsPb(const std::string& path,
                          Scope* scope,
                          const ProgramDesc& prog,
                          bool params_from_memory = false);

// Read a model and files of parameters in pb format.
void LoadModelPb(const std::string& model_dir,
                 const std::string& model_file,
                 const std::string& param_file,
                 Scope* scope,
                 ProgramDesc* prog,
                 bool combined = false,
                 bool model_from_memory = false);

// Save a model and files of parameters in pb format.
void SaveModelPb(const std::string& model_dir, const Scope& scope, const ProgramDesc& prog, bool combined = false);

void SaveCombinedParamsPb(const std::string& path, const Scope& exec_scope, const ProgramDesc& prog);

// Serialize tensors to ostream.
void SerializeTensor(std::ostream& os, const Scope& scope, const std::string& var);

// LoDTensor to ostream
void TensorToStream(std::ostream& os, const Tensor& tensor);
void TensorFromStream(std::istream& is, Tensor* tensor);
void ReadBinaryFile(const std::string& filename, std::string* contents);

}  // namespace fluid
}  // namespace model_loader
}  // namespace cinn
