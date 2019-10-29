# CINN -- Compiler Infrusture for Neural Networks

## Build
The compatible OS is Ubuntu16.04 and Mac.

On Ubuntu16.04, the following packages are required:

- git
- cmake
- zlib1g-dev
- libclang-6.0-dev
- libtool
- autoconf
- wget

the ISL should be installed with the following commands:

```sh
git clone https://github.com/Meinersbur/isl.git
cd isl
./autogen.sh
./configure --clang=system
make -j20
make install
```
