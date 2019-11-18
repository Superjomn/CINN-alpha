#include "cinn/api/python_api.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/hlir/builder.h"
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/network.h"
#include "cinn/hlir/optimize/use_passes.h"

using namespace cinn;        // NOLINT
using namespace cinn::hlir;  // NOLINT
namespace py = pybind11;

void BindSession(py::module* m) {
  py::class_<Session>(*m, "Session")  //
      .def(py::init<>());
}

void BindVar(py::module* m) {
  py::class_<Network::Var>(*m, "Var")       //
      .def(py::init<const std::string&>())  //
      .def_readwrite("name", &Network::Var::name);
}

void BindPrimitiveT(py::module* m) {
  py::enum_<primitive_t>(*m, "primitive_t")  //
#define __(type__) .value(#type__, primitive_t::type__)
      __(unk)      //
      __(uint8)    //
      __(uint16)   //
      __(int32)    //
      __(uint64)   //
      __(int8)     //
      __(int16)    //
      __(int32)    //
      __(int64)    //
      __(float32)  //
      __(float64)  //
      __(boolean)  //
      __(void_);
#undef __
}

void BindShape(py::module* m) {
  py::class_<Shape>(*m, "Shape")                  //
      .def(py::init<const std::vector<int>&>())   //
      .def_readwrite("date", &Shape::data)        //
      .def("num_elements", &Shape::num_elements)  //
      .def("num_bytes", &Shape::num_bytes)        //
      .def("size", &Shape::size);
}

void BindNetwork(py::module* m) {
  py::class_<Network>(*m, "Network")                            //
      .def(py::init<const std::string&, Session*>())            //
      .def("decl_input", &Network::DeclInput)                   //
      .def("decl_output", &Network::DeclOutput)                 //
      .def("decl_weight", &Network::DeclWeight<float>)          //
      .def("decl_weight_fp32", &Network::DeclWeight<float>)     //
      .def("decl_weight_int32", &Network::DeclWeight<int32_t>)  //
      .def("add_fc", &Network::AddFc)                           //
      .def("add_mat_mul", &Network::AddMatMul)                  //
      .def("add_tanh", &Network::AddTanh)                       //
      .def("add_elementwise_add", &Network::AddElementwiseAdd)  //
      .def("add_elementwise_sub", &Network::AddElementwiseSub)  //
      .def("add_elementwise_div", &Network::AddElementwiseDiv)  //
      .def("add_elementwise_mul", &Network::AddElementwiseMul)  //
      .def("num_operators", &Network::num_operators);
}

void BindBuilder(py::module* m) {
  py::class_<Builder>(*m, "Builder")
      .def(py::init<>())              //
      .def("build", &Builder::Build)  //
      .def("to_c_source_code", &Builder::ToCSourceCode);
}

void BindExpr(py::module* m) {
  py::class_<ir::Expr>(*m, "Expr")  //
      .def(py::init<>());
}

void BindApi(pybind11::module* m) {
  BindSession(m);
  BindVar(m);
  BindExpr(m);
  BindPrimitiveT(m);
  BindShape(m);
  BindNetwork(m);
  BindBuilder(m);
}
