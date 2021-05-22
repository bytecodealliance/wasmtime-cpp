/*
Example of instantiating of the WebAssembly module and invoking its exported
function.

You can compile and run this example on Linux with:

   cargo build --release -p wasmtime-c-api
   c++ examples/fuel.cc -std=c++20 \
       -I crates/c-api/include \
       -I crates/c-api/wasm-c-api/include \
       target/release/libwasmtime.a \
       -lpthread -ldl -lm \
       -o fuel
   ./fuel

Note that on Windows and macOS the command will be similar, but you'll need
to tweak the `-lpthread` and such annotations.
*/

#include <fstream>
#include <iostream>
#include <sstream>
#include <wasmtime.hh>

using namespace wasmtime;

template<typename T, typename E>
T unwrap(Result<T, E> result) {
  if (result)
    return result.ok();
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
  Config config;
  config.consume_fuel(true);
  Engine engine(std::move(config));
  Store store(engine);
  store.context().add_fuel(10000);

  auto wat = readFile("examples/fuel.wat");
  Module module = unwrap(Module::compile(engine, wat));
  Instance instance = unwrap(Instance::create(store, module, {}));
  Func fib = std::get<Func>(*instance.get(store, "fibonacci"));

  // Call it repeatedly until it fails
  for (int32_t n = 1; ; n++) {
    uint64_t fuel_before = *store.context().fuel_consumed();
    auto result = fib.call(store, {n});
    if (!result) {
      std::cout << "Exhausted fuel computing fib(" << n << ")\n";
      break;
    }
    uint64_t consumed = *store.context().fuel_consumed() - fuel_before;;
    auto fib_result = unwrap(std::move(result))[0].i32();

    std::cout << "fib(" << n << ") = " << fib_result << " [consumed " << consumed << " fuel]\n";
    unwrap(store.context().add_fuel(consumed));
  }
}
