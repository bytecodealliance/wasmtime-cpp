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
  // Create our `store` context and then compile a module and create an
  // instance from the compiled module all in one go.
  Engine engine;
  Module module = unwrap(Module::compile(engine, readFile("examples/memory.wat")));
  Store store(engine);
  Instance instance = unwrap(Instance::create(store, module, {}));

  // load_fn up our exports from the instance
  auto memory = std::get<Memory>(*instance.get(store, "memory"));
  auto size = std::get<Func>(*instance.get(store, "size"));
  auto load_fn = std::get<Func>(*instance.get(store, "load"));
  auto store_fn = std::get<Func>(*instance.get(store, "store"));

  std::cout << "Checking memory...\n";
  assert(memory.size(store) == 2);
  auto data = memory.data(store);
  assert(data.size() == 0x20000);
  assert(data[0] == 0);
  assert(data[0x1000] == 1);
  assert(data[0x1003] == 4);

  assert(unwrap(size.call(store, {}))[0].i32() == 2);
  assert(unwrap(load_fn.call(store, {0}))[0].i32() == 0);
  assert(unwrap(load_fn.call(store, {0x1000}))[0].i32() == 1);
  assert(unwrap(load_fn.call(store, {0x1003}))[0].i32() == 4);
  assert(unwrap(load_fn.call(store, {0x1ffff}))[0].i32() == 0);
  load_fn.call(store, {0x20000}).err(); // out of bounds trap

  std::cout << "Mutating memory...\n";
  memory.data(store)[0x1003] = 5;

  unwrap(store_fn.call(store, {0x1002, 6}));
  store_fn.call(store, {0x20000, 0}).err(); // out of bounds trap

  assert(memory.data(store)[0x1002] == 6);
  assert(memory.data(store)[0x1003] == 5);
  assert(unwrap(load_fn.call(store, {0x1002}))[0].i32() == 6);
  assert(unwrap(load_fn.call(store, {0x1003}))[0].i32() == 5);

  // Grow memory.
  std::cout << "Growing memory...\n";
  unwrap(memory.grow(store, 1));
  assert(memory.size(store) == 3);
  assert(memory.data(store).size() == 0x30000);

  assert(unwrap(load_fn.call(store, {0x20000}))[0].i32() == 0);
  unwrap(store_fn.call(store, {0x20000, 0}));
  load_fn.call(store, {0x30000}).err();
  store_fn.call(store, {0x30000, 0}).err();

  memory.grow(store, 1).err();
  memory.grow(store, 0).ok();

  std::cout << "Creating stand-alone memory...\n";
  MemoryType ty(Limits(5, 5));
  Memory memory2 = unwrap(Memory::create(store, ty));
  assert(memory2.size(store) == 5);
  memory2.grow(store, 1).err();
  memory2.grow(store, 0).ok();
}
