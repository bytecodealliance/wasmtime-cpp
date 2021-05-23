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
  std::cout << "Initializing...\n";
  Engine engine;
  Store store(engine);

  std::cout << "Compiling module...\n";
  auto wat = readFile("examples/externref.wat");
  Module module = unwrap(Module::compile(engine, wat));
  std::cout << "Instantiating module...\n";
  Instance instance = unwrap(Instance::create(store, module, {}));

  ExternRef externref(std::string("Hello, world!"));
  std::any &data = externref.data();
  std::cout << "externref data: " << std::any_cast<std::string>(data) << "\n";

  std::cout << "Touching `externref` table..\n";
  Table table = std::get<Table>(*instance.get(store, "table"));
  unwrap(table.set(store, 3, externref));
  ExternRef val = *table.get(store, 3)->externref();
  std::cout << "externref data: " << std::any_cast<std::string>(val.data()) << "\n";

  std::cout << "Touching `externref` global..\n";
  Global global = std::get<Global>(*instance.get(store, "global"));
  unwrap(global.set(store, externref));
  val = *global.get(store).externref();
  std::cout << "externref data: " << std::any_cast<std::string>(val.data()) << "\n";

  std::cout << "Calling `externref` func..\n";
  Func func = std::get<Func>(*instance.get(store, "func"));
  auto results = unwrap(func.call(store, {externref}));
  val = *results[0].externref();
  std::cout << "externref data: " << std::any_cast<std::string>(val.data()) << "\n";


  std::cout << "Running a gc..\n";
  store.context().gc();
}
