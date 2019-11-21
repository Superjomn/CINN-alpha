import sys
import os

path = os.getenv('cinn_so_prefix', None)
assert path, "the environment variable %s should be set first" % path
sys.path.append(path)
import cinn_core as cinn

cinn.init_global_context()

builder = cinn.Builder()

session = cinn.Session()

net = cinn.Network("test", session)

x_shape = cinn.Shape([32, 2])
x0 = net.decl_input("x0", cinn.primitive_t.float32, x_shape)
b = net.decl_weight("b", cinn.primitive_t.float32, cinn.Shape([16]), [0.1 * i for i in range(16)])
w = net.decl_weight("w", cinn.primitive_t.float32, cinn.Shape([2, 16]), [0.1 * i for i in range(64)])

out = net.add_fc(x0, w, b)
out = net.add_tanh(out)

expr = builder.build(session, net)
builder.to_c_source_code(expr, "python_gen")

code_gen = open('python_gen.cc').read()

target = '''
#ifndef CINN_FILE_
#define CINN_FILE_
#include <immintrin.h>
#include <math.h>
#include <stdio.h>

typedef char cinn_int8_t;
typedef int cinn_int32_t;
typedef long long cinn_int64_t;
typedef unsigned char cinn_uint8_t;
typedef unsigned int cinn_uint32_t;
typedef unsigned long long cinn_uint64_t;
typedef float cinn_float32_t;

#define cinn_min(a,b) ((a)<(b) ? (a) : (b))
#define cinn_max(a,b) ((a)>(b) ? (a) : (b))
#define cinn_copy(a,b,size) memcpy((b), (a), (size))


// create weight buffers
cinn_float32_t b[] = {0.000000,0.100000,0.200000,0.300000,0.400000,0.500000,0.600000,0.700000,0.800000,0.900000,1.000000,1.100000,1.200000,1.300000,1.400000,1.500000};
cinn_float32_t w[] = {0.000000,0.100000,0.200000,0.300000,0.400000,0.500000,0.600000,0.700000,0.800000,0.900000,1.000000,1.100000,1.200000,1.300000,1.400000,1.500000,1.600000,1.700000,1.800000,1.900000,2.000000,2.100000,2.200000,2.300000,2.400000,2.500000,2.600000,2.700000,2.800000,2.900000,3.000000,3.100000};
// create input buffers
cinn_float32_t* x0 =  (cinn_float32_t*) malloc(256);
// create output buffers
// create temporary variable buffers
cinn_float32_t* tmp0 =  (cinn_float32_t*) malloc(2048);
cinn_float32_t* tmp1 =  (cinn_float32_t*) malloc(2048);
cinn_float32_t* tmp2 =  (cinn_float32_t*) malloc(2048);

// functions for reading output data
// functions for loadding input data
void set_input_x0 (cinn_float32_t* x0_) {
  cinn_copy(x0_, x0, 256);
}
void func8 (cinn_float32_t* b, cinn_float32_t* w, cinn_float32_t* x0, cinn_float32_t* tmp2) {
  for (int c0 = 0; (c0 <= 31); c0 += 1) {
    for (int c1 = 0; (c1 <= 15); c1 += 1) {
      for (int c2 = 0; (c2 <= 1); c2 += 1) {
        tmp0[((c0 * 16) + c1)] += (x0[((c0 * 2) + c2)] * w[((c2 * 16) + c1)]);
      }
      tmp1[((c0 * 16) + c1)] = (tmp0[((c0 * 16) + c1)] + b[c1]);
      tmp2[((c0 * 16) + c1)] = cinn_max(tmp1[((c0 * 16) + c1)], 0);
    }
  }
}
void main_ () {
  func8(b, w, x0, tmp2);
}

#endif  // CINN_FILE_
'''

assert code_gen.strip() == target.strip()
