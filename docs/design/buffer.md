# Buffer Design

Buffer represents a continus memory used to hold data.



The relation between buffer and tensor:

- Each tensor holds a buffer,
- Multiple tensor can share a same buffer, the external logic should make sure the WAR, WAW,



## Buffer declaration

```c++
struct Buffer : public ir::ExprNode<Buffer> {
	DeviceInfo device; // CPU or GPU
  int size;  // Can deduce from the tensors, set to the maximum value if shared by multiple tensor.
  int align; // memory align of the buffer.
  std::string name; // used in CodeGen for better readability.
};
```

Buffer is a IR Node, it will lower to the actual code or LLVM IR.



## Bind a Buffer to a Tensor 

By default, each tensor holds a unique buffer, but some optimization can take place in this phase, such as

- Reusing buffer between different tensors
- ...

To assure that the precession is not affected by reusage, the optimization will take in the graph optimizer.

## Buffer lowering

To make the Buffer IR more transparent for the underlying code generation or LLVM IR, the allocation and dallocation  will use the raw operation in the programming language.

e.g. the `malloc` and `free` will be used in C language.

