<div align="center">
  <h1><code>wasmtime-cpp</code></h1>

  <p>
    <strong>C++ embedding of
    <a href="https://github.com/bytecodealliance/wasmtime">Wasmtime</a></strong>
  </p>

  <strong>A <a href="https://bytecodealliance.org/">Bytecode Alliance</a> project</strong>

  <p>
    <a href="https://github.com/bytecodealliance/wasmtime-cpp/actions?query=workflow%3ACI">
      <img src="https://github.com/bytecodealliance/wasmtime-cpp/workflows/CI/badge.svg" alt="CI status"/>
    </a>
    <a href="https://bytecodealliance.github.io/wasmtime-cpp/">
      <img src="https://img.shields.io/badge/docs-main-green" alt="Documentation"/>
    </a>
  </p>

</div>

## Installation

To install the C++ API from precompiled binaries you'll need a few things;

* First you'll want wasmtime's [C API](https://docs.wasmtime.dev/c-api/)
  installed. This is easiest by downloading a [precompiled
  release](https://github.com/bytecodealliance/wasmtime/releases) (an artifact
  with "c-api" in the name).

* Next you'll want to copy [`include/wasmtime.hh`](include/wasmtime.hh) into
  the directory with `wasmtime.h` from the C API.

Afterwards you can also consult the [linking
documentation](https://docs.wasmtime.dev/c-api/) for the C API to link Wasmtime
into your project. Your C++ code will then use:

```cpp
#include <wasmtime.hh>
```

## Build Requirements

Wasmtime supports Windows, macOS, and Linux. Wasmtime also supports the AArch64
and x86\_64 architectures.

The C++ header file here requires C++17 support, or the `-std=c++17` flag for
unix compilers.

## Contributing

See [`CONTRIBUTING.md`](./CONTRIBUTING.md).
