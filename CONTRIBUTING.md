# Contributing to `wasmtime-cpp`

`wasmtime-cpp` is a [Bytecode Alliance] project. It follows the Bytecode
Alliance's [Code of Conduct] and [Organizational Code of Conduct].

So far this extension has been written by folks who are primarily Rust
programmers, so feel free to create a PR to help make things more idiomatic if
you see something!

## Building

You'll need to acquire a [Wasmtime] installation. You can do this by extracting
the C API of Wasmtime into a top-level folder called `c-api` in this repository.
After doing this `c-api/include/wasmtime.h` should exist.

[wasmtime]: https://wasmtime.dev/

Afterwards you can configure the C++ build with CMake:

```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

Building will build all examples and tests. This is a header-only library which
uses the C-API as a precompiled binary, so there's nothing to build for the
library itself. This me

## Testing

To run tests you can use CMake's `ctest` command inside the build directory.
Note that this does not automatically rebuild binaries, so you may wish to run
the build command first.

```
$ ctest
```

### CI and Releases

The CI for this project does a few different things. First it generates API docs
for pushes to the `main` branch and are [published online][apidoc]. It also runs
all tests against many supported platforms. Additionally `clang-format` is used
for formatting `wasmtime.hh` and `clang-tidy` is used for more lint checks.

[Bytecode Alliance]: https://bytecodealliance.org/
[Code of Conduct]: CODE_OF_CONDUCT.md
[Organizational Code of Conduct]: ORG_CODE_OF_CONDUCT.md
[Wasmtime]: https://github.com/bytecodealliance/wasmtime
[apidoc]: https://bytecodealliance.github.io/wasmtime-cpp/
