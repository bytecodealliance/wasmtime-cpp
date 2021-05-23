#include <fstream>
#include <iostream>
#include <sstream>
#include <wasmtime.hh>

using namespace wasmtime;

template<typename T, typename E>
T unwrap(Result<T, E> result) {
  if (result) {
    return result.ok();
  }
  std::cerr << "error: " << result.err().message() << "\n";
  std::abort();
}

std::string readFile(const char* name) {
  std::ifstream watFile;
  watFile.open(name);
  std::stringstream strStream;
  strStream << watFile.rdbuf();
  return strStream.str();
}

int main() {
  Engine engine;
  Store store(engine);

  // Read our input `*.wat` files into `std::string`s
  std::string linking1_wat = readFile("examples/linking1.wat");
  std::string linking2_wat = readFile("examples/linking2.wat");

  // Compile our two modules
  Module linking1_module = unwrap(Module::compile(engine, linking1_wat));
  Module linking2_module = unwrap(Module::compile(engine, linking2_wat));

  // Configure WASI and store it within our `wasmtime_store_t`
  WasiConfig wasi;
  wasi.inherit_argv();
  wasi.inherit_env();
  wasi.inherit_stdin();
  wasi.inherit_stdout();
  wasi.inherit_stderr();
  unwrap(store.context().set_wasi(std::move(wasi)));

  // Create our linker which will be linking our modules together, and then add
  // our WASI instance to it.
  Linker linker(engine);
  unwrap(linker.define_wasi());

  // Instantiate our first module which only uses WASI, then register that
  // instance with the linker since the next linking will use it.
  Instance linking2 = unwrap(linker.instantiate(store, linking2_module));
  unwrap(linker.define_instance(store, "linking2", linking2));

  // And with that we can perform the final link and the execute the module.
  Instance linking1 = unwrap(linker.instantiate(store, linking1_module));
  Func f = std::get<Func>(*linking1.get(store, "run"));
  unwrap(f.call(store, {}));
}
