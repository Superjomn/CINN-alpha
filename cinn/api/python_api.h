#pragma once

#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

void BindApi(pybind11::module *m);

PYBIND11_MODULE(cinn_core, m) {
  m.doc() = "C++ core of CINN";

  BindApi(&m);
}
