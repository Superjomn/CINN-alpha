# High Level IR(HLIR)

The CINN compiler takes a real model as input, and generate code or binary library as output. A real world model is a complex computation graph, and there are many ways to optimize it.

Just like a general deep learning framework, such as Paddle Lite, CINN will turn a DNN model into the corresponding SSA graph, and take some high level analysis and optimization on it.

There are some key modules to power such operations:

- `Graph` which is a SSA graph, it contains two kinds of nodes, one is `Operator` node, the other is `Tensor` node.
- `Tensor` which represents the variable of the operators,
- `Operator` which is similar to the Operator concept in general DNN platforms.

The `Operator` takes one or more `Tensor`s as input, and output several `Tensors` as output. The implementation of each `Operator` is several stages of `ir::Expr` , and the stages are appended to its output `Tensor`.

## Graph

### `Graph` Demo

<img src="hlir.assets/cinn-hlir-design.png" alt="cinn-hlir-design" style="zoom:67%;" />

### Partition the graph to functions

