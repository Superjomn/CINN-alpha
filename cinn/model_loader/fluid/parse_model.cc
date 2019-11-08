#include "cinn/model_loader/fluid/parse_model.h"
#include <fstream>
#include "cinn/model_loader/fluid/block_desc.h"
#include "cinn/model_loader/fluid/var_desc.h"
#include "cinn/model_loader/tensor.h"

namespace cinn {
namespace model_loader {
namespace fluid {

using namespace paddle;  // NOLINT

int SizeOfType(framework::proto::VarType::Type type) {
  using Type = framework::proto::VarType::Type;
  switch (static_cast<int>(type)) {
#define DO(desc, type)            \
  case Type::VarType_Type_##desc: \
    return sizeof(type);
    DO(BOOL, bool);
    DO(FP16, float);
    DO(FP32, float);
    DO(INT8, int8_t);
    DO(INT32, int);
    DO(INT64, int64_t);
#undef DO
    default:
      LOG(FATAL) << "unknown data type " << type;
  }
  return -1;
}

void TensorFromStream(std::istream &is, Tensor *tensor) {
  using Type = framework::proto::VarType::Type;
  uint32_t version;
  is.read(reinterpret_cast<char *>(&version), sizeof(version));
  CHECK_EQ(version, 0U) << "Only version 0 is supported";
  // read tensor desc
  framework::proto::VarType::TensorDesc desc;
  {
    // int32_t size
    // proto buffer
    int32_t size;
    is.read(reinterpret_cast<char *>(&size), sizeof(size));
    std::unique_ptr<char[]> buf(new char[size]);
    is.read(reinterpret_cast<char *>(buf.get()), size);
    CHECK(desc.ParseFromArray(buf.get(), size)) << "Cannot parse tensor desc";
  }

  // read tensor
  std::vector<int64_t> dims_vec;
  std::copy(desc.dims().begin(), desc.dims().end(), std::back_inserter(dims_vec));
  DDim dims(dims_vec);
  tensor->Resize(dims);
  void *buf;
  size_t size = tensor->dims().production() * SizeOfType(desc.data_type());
  // alllocate memory
  switch (static_cast<int>(desc.data_type())) {
#define SET_TENSOR(desc, type, precision)          \
  case Type::VarType_Type_##desc:                  \
    buf = tensor->mutable_data<type>();            \
    tensor->set_precision(primitive_t::precision); \
    break

    // SET_TENSOR(BOOL, bool, PRECISION(kBool));
    SET_TENSOR(FP32, float, float32);
    SET_TENSOR(INT8, int8_t, int8);
    SET_TENSOR(INT16, int16_t, int16);
    SET_TENSOR(INT32, int32_t, int32);
    SET_TENSOR(INT64, int64_t, int64);
#undef SET_TENSOR
    default:
      LOG(FATAL) << "unknown type " << desc.data_type();
  }
  tensor->set_persistable(true);

  is.read(static_cast<char *>(buf), size);
}

void LoadLoDTensor(std::istream &is, Variable *var) {
  auto *tensor = var->GetMutable<Tensor>();
  uint32_t version{};
  is.read(reinterpret_cast<char *>(&version), sizeof(version));
  VLOG(3) << "model version " << version;

  // Load LoD information
  uint64_t lod_level{};
  is.read(reinterpret_cast<char *>(&lod_level), sizeof(lod_level));
  auto &lod = *tensor->mutable_lod();
  lod.resize(lod_level);
  for (uint64_t i = 0; i < lod_level; ++i) {
    uint64_t size;
    is.read(reinterpret_cast<char *>(&size), sizeof(size));
    std::vector<uint64_t> tmp(size / sizeof(uint64_t));
    is.read(reinterpret_cast<char *>(tmp.data()), static_cast<std::streamsize>(size));
    lod[i] = tmp;
  }

  TensorFromStream(is, tensor);
}

void ReadBinaryFile(const std::string &filename, std::string *contents) {
  std::ifstream fin(filename, std::ios::in | std::ios::binary);
  CHECK(fin.is_open()) << "Cannot open file: " << filename;
  fin.seekg(0, std::ios::end);
  auto size = fin.tellg();
  contents->clear();
  contents->resize(size);
  fin.seekg(0, std::ios::beg);
  fin.read(&(contents->at(0)), contents->size());
  fin.close();
}

std::unique_ptr<framework::proto::ProgramDesc> LoadProgram(const std::string &path, bool program_from_memory) {
  std::unique_ptr<framework::proto::ProgramDesc> main_program(new framework::proto::ProgramDesc);
  if (!program_from_memory) {
    std::string desc_str;
    ReadBinaryFile(path, &desc_str);
    main_program->ParseFromString(desc_str);
  } else {
    main_program->ParseFromString(path);
  }
  return main_program;
}

void LoadParams(const std::string &path) {}

// Load directly to CPU, and latter transfer to other devices.
void LoadParam(const std::string &path, Variable *out) {
  std::ifstream fin(path, std::ios::binary);
  CHECK(fin.is_open()) << "failed to open file " << path;
  LoadLoDTensor(fin, out);
}

bool IsPersistable(const VarDesc &var) {
  if (var.Persistable() && var.GetType() != VarType::FEED_MINIBATCH && var.GetType() != VarType::FETCH_LIST &&
      var.GetType() != VarType::RAW) {
    return true;
  }
  return false;
}

void LoadCombinedParamsPb(const std::string &path, Scope *scope, const ProgramDesc &cpp_prog, bool params_from_memory) {
  CHECK(scope);
  auto prog = cpp_prog;
  auto &main_block_desc = *prog.GetBlock<BlockDesc>(0);

  // Get vars
  std::vector<std::string> paramlist;
  for (size_t i = 0; i < main_block_desc.VarsSize(); ++i) {
    auto &var = *main_block_desc.GetVar<VarDesc>(i);
    if (!IsPersistable(var)) continue;
    paramlist.push_back(var.Name());
  }
  std::sort(paramlist.begin(), paramlist.end());

  // Load vars
  auto load_var_func = [&](std::istream &is) {
    for (size_t i = 0; i < paramlist.size(); ++i) {
      auto *var = scope->Var(paramlist[i]);
      // Error checking
      CHECK(static_cast<bool>(is)) << "There is a problem with loading model parameters";
      LoadLoDTensor(is, var);
    }
    is.peek();
    CHECK(is.eof()) << "You are not allowed to load partial data via"
                    << " LoadCombinedParamsPb, use LoadParam instead.";
  };

  if (params_from_memory) {
    std::stringstream fin(path, std::ios::in | std::ios::binary);
    load_var_func(fin);
  } else {
    std::ifstream fin(path, std::ios::binary);
    CHECK(fin.is_open());
    load_var_func(fin);
  }
}

void LoadModelPb(const std::string &model_dir,
                 const std::string &model_file,
                 const std::string &param_file,
                 Scope *scope,
                 ProgramDesc *cpp_prog,
                 bool combined,
                 bool model_from_memory) {
  CHECK(cpp_prog);
  CHECK(scope);
  cpp_prog->ClearBlocks();

  // Load model
  VLOG(4) << "Start load model program...";
  std::string prog_path = model_dir + "/__model__";
  if (combined) {
    prog_path = model_file;
  }
  framework::proto::ProgramDesc pb_proto_prog = *LoadProgram(prog_path, model_from_memory);
  *cpp_prog = ProgramDesc(&pb_proto_prog);

  // Load Params
  // NOTE: Only main block be used now.
  VLOG(4) << "Start load model params...";
  CHECK(!(!combined && model_from_memory)) << "If you want use the model_from_memory,"
                                           << " you should load the combined model using cfg.set_model_buffer "
                                              "interface.";
  if (combined) {
    LoadCombinedParamsPb(param_file, scope, *cpp_prog, model_from_memory);
  } else {
    auto main_block = pb_proto_prog.blocks(0);
    for (auto &var : main_block.vars()) {
      if (var.name() == "feed" || var.name() == "fetch" || !var.persistable()) continue;

      std::string file_path = model_dir + "/" + var.name();
      VLOG(4) << "reading weight " << var.name();

      std::ifstream file(file_path);
      switch (var.type().type()) {
        case framework::proto::VarType_Type_LOD_TENSOR:
          LoadLoDTensor(file, scope->Var(var.name()));
          break;
        default:
          CHECK(false) << "unknown weight type";
      }
    }
  }

  VLOG(4) << "Load protobuf model in '" << model_dir << "'' successfully";
}

void TensorToStream(std::ostream &os, const Tensor &tensor) {
  // the 1st field, uint32_t version
  constexpr uint32_t version = 0;
  os.write(reinterpret_cast<const char *>(&version), sizeof(version));

  {
    uint64_t size = tensor.lod().size();
    // the 2st field, LoD information
    // uint64_t lod_level
    // uint64_t lod_level_1 size in byte.
    // int*     lod_level_1 data
    // ...
    os.write(reinterpret_cast<const char *>(&size), sizeof(size));

    for (auto &each : tensor.lod()) {
      size = each.size() * sizeof(each.front());
      os.write(reinterpret_cast<const char *>(&size), sizeof(size));
      os.write(reinterpret_cast<const char *>(each.data()), static_cast<std::streamsize>(size));
    }
  }

  // There are two version fields in a LoDTensor.
  os.write(reinterpret_cast<const char *>(&version), sizeof(version));

  {  // the 2nd field, tensor description
    // int32_t  size
    // void*    protobuf message
    framework::proto::VarType::TensorDesc desc;
    // TODO(Superjomn) support other data types.
    switch (tensor.precision()) {
#define SET_DATA_TYPE(precision, type_desc) \
  case primitive_t::precision:              \
    desc.set_data_type(type_desc);          \
    break

      SET_DATA_TYPE(float32, framework::proto::VarType_Type_FP32);
      SET_DATA_TYPE(int8, framework::proto::VarType_Type_INT8);
      SET_DATA_TYPE(int16, framework::proto::VarType_Type_INT16);
      SET_DATA_TYPE(int32, framework::proto::VarType_Type_INT32);
      SET_DATA_TYPE(int64, framework::proto::VarType_Type_INT64);
#undef SET_DATA_TYPE
      default:
        LOG(FATAL) << "unknown precision type: " << tensor.precision();
    }
    auto dims = tensor.dims();
    auto *pb_dims = desc.mutable_dims();
    pb_dims->Resize(static_cast<int>(dims.size()), 0);
    auto dims_vec = dims.Vectorize();
    std::copy(dims_vec.begin(), dims_vec.end(), pb_dims->begin());
    int32_t size = desc.ByteSize();
    os.write(reinterpret_cast<const char *>(&size), sizeof(size));
    auto out = desc.SerializeAsString();
    os.write(out.data(), size);
  }
  {  // the 3rd field, tensor data
    uint64_t size = tensor.memory_size();
    CHECK_LT(size, std::numeric_limits<std::streamsize>::max()) << "Index overflow when writing tensor";

#ifdef LITE_WITH_CUDA
    if (tensor.target() == TARGET(kCUDA)) {
      std::unique_ptr<char> tmp_buffer(new char[size]);
      TargetWrapperCuda::MemcpySync(tmp_buffer.get(), tensor.data<float>(), tensor.data_size(), IoDirection::DtoH);
      os.write(static_cast<const char *>(tmp_buffer.get()), static_cast<std::streamsize>(size));
    } else  // NOLINT
#endif      // LITE_WITH_CUDA
    {
      os.write(static_cast<const char *>(tensor.data<void>()), static_cast<std::streamsize>(size));
    }
  }
}

void SerializeTensor(std::ostream &os, const Scope &scope, const std::string &var_name) {
  // Store all the persistable vars.
  auto *var = scope.FindVar(var_name);
  const auto &tensor = var->Get<Tensor>();
  TensorToStream(os, tensor);
}

}  // namespace fluid
}  // namespace model_loader
}  // namespace cinn
