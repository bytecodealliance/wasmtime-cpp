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
  // Load our WebAssembly (parsed WAT in our case), and then load it into a
  // `Module` which is attached to a `Store`. After we've got that we
  // can instantiate it.
  Engine engine;
  Store store(engine);
  auto module = unwrap(Module::compile(engine, readFile("examples/gcd.wat")));
  auto instance = unwrap(Instance::create(store, module, {}));

  // Invoke `gcd` export
  auto gcd = std::get<Func>(*instance.get(store, "gcd"));
  auto results = unwrap(gcd.call(store, {6, 27}));

  std::cout << "gcd(6, 27) = " << results[0].i32() << "\n";
}
