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

expr = builder.build(session, net)
builder.to_c_source_code(expr, "python_gen")
