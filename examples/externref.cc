#include <fstream>
#include <iostream>
#include <sstream>
#include <wasmtime.hh>

using namespace wasmtime;

std::string readFile(const char* name) {
  std::ifstream watFile;
  watFile.open(name);
  std::stringstream strStream;
  strStream << watFile.rdbuf();
  return strStream.str();
}

int main() {
#if WASMTIME_HAS_EXTERNREF
  std::cout << "Initializing...\n";
  Engine engine;
  Store store(engine);

  std::cout << "Compiling module...\n";
  auto wat = readFile("examples/externref.wat");
  Module module = Module::compile(engine, wat).unwrap();
  std::cout << "Instantiating module...\n";
  Instance instance = Instance::create(store, module, {}).unwrap();

  ExternRef externref(std::string("Hello, world!"));
  std::any &data = externref.data();
  std::cout << "externref data: " << std::any_cast<std::string>(data) << "\n";

  std::cout << "Touching `externref` table..\n";
  Table table = std::get<Table>(*instance.get(store, "table"));
  table.set(store, 3, externref).unwrap();
  ExternRef val = *table.get(store, 3)->externref();
  std::cout << "externref data: " << std::any_cast<std::string>(val.data()) << "\n";

  std::cout << "Touching `externref` global..\n";
  Global global = std::get<Global>(*instance.get(store, "global"));
  global.set(store, externref).unwrap();
  val = *global.get(store).externref();
  std::cout << "externref data: " << std::any_cast<std::string>(val.data()) << "\n";

  std::cout << "Calling `externref` func..\n";
  Func func = std::get<Func>(*instance.get(store, "func"));
  auto results = func.call(store, {externref}).unwrap();
  val = *results[0].externref();
  std::cout << "externref data: " << std::any_cast<std::string>(val.data()) << "\n";


  std::cout << "Running a gc..\n";
  store.context().gc();
#else
  std::cout << "no support for externref at compile time\n";
#endif
}
