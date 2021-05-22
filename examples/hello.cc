/*
Example of instantiating of the WebAssembly module and invoking its exported
function.

You can compile and run this example on Linux with:

   cargo build --release -p wasmtime-c-api
   cc examples/hello.c \
       -I crates/c-api/include \
       -I crates/c-api/wasm-c-api/include \
       target/release/libwasmtime.a \
       -lpthread -ldl -lm \
       -o hello
   ./hello

Note that on Windows and macOS the command will be similar, but you'll need
to tweak the `-lpthread` and such annotations as well as the name of the
`libwasmtime.a` file on Windows.
*/

#include <fstream>
#include <iostream>
#include <sstream>
#include <wasmtime.hh>

using namespace wasmtime;

/* static void exit_with_error(const char *message, wasmtime_error_t *error, wasm_trap_t *trap); */

/* static wasm_trap_t* hello_callback( */
/*     void *env, */
/*     wasmtime_caller_t *caller, */
/*     const wasmtime_val_t *args, */
/*     size_t nargs, */
/*     wasmtime_val_t *results, */
/*     size_t nresults */
/* ) { */
/*   printf("Calling back...\n"); */
/*   printf("> Hello World!\n"); */
/*   return NULL; */
/* } */

int main() {
  Engine engine;
  Engine engine2(Config);
  Config c;
  Engine engine3(std::move(c));
  Store store(engine);

  // Read our input `*.wat` file to a `std::string`
  std::ifstream watFile;
  watFile.open("examples/hello.wat");
  std::stringstream strStream;
  strStream << watFile.rdbuf();
  std::string wat = strStream.str();

  // Now that we've got our binary wasm we can compile our module.
  printf("Compiling module...\n");
  auto maybeModule = Module::compile(engine, wat);
  if (!maybeModule) {
    std::cerr << "failed to compile module: " << maybeModule.err();
    std::exit(1);
  }

  Module module = maybeModule.ok();

  ValType t = ValType::i32();
  t->kind();
  store.context().gc();
  store.context().set_wasi(WasiConfig());
  store.context().interrupt_handle();

  ExternRef d((float) 3);
  std::cout << d.data().type().name() << "\n";

  /* FuncType t({ValType::i32()}, {}); */
  /* for (ValTypeRef v : t.params()) { */
  /*   v.kind(); */
  /* } */
  /* for (ValTypeRef v : t.results()) { */
  /*   v.kind(); */
  /* } */
  /* Trap t("wat"); */
  /* std::cout << t.message() <<  t.message().size() << "\n"; */

  // Next up we need to create the function that the wasm module imports. Here
  // we'll be hooking up a thunk function to the `hello_callback` native
  // function above. Note that we can assign custom data, but we just use NULL
  // for now).
  /* printf("Creating callback...\n"); */
  /* wasm_functype_t *hello_ty = wasm_functype_new_0_0(); */
  /* wasmtime_func_t hello; */
  /* wasmtime_func_new(context, hello_ty, hello_callback, NULL, NULL, &hello); */

  /* // With our callback function we can now instantiate the compiled module, */
  /* // giving us an instance we can then execute exports from. Note that */
  /* // instantiation can trap due to execution of the `start` function, so we need */
  /* // to handle that here too. */
  /* printf("Instantiating module...\n"); */
  /* wasm_trap_t *trap = NULL; */
  /* wasmtime_instance_t instance; */
  /* wasmtime_extern_t import; */
  /* import.kind = WASMTIME_EXTERN_FUNC; */
  /* import.of.func = hello; */
  /* error = wasmtime_instance_new(context, module, &import, 1, &instance, &trap); */
  /* if (error != NULL || trap != NULL) */
  /*   exit_with_error("failed to instantiate", error, trap); */

  /* // Lookup our `run` export function */
  /* printf("Extracting export...\n"); */
  /* wasmtime_extern_t run; */
  /* bool ok = wasmtime_instance_export_get(context, &instance, "run", 3, &run); */
  /* assert(ok); */
  /* assert(run.kind == WASMTIME_EXTERN_FUNC); */

  /* // And call it! */
  /* printf("Calling export...\n"); */
  /* error = wasmtime_func_call(context, &run.of.func, NULL, 0, NULL, 0, &trap); */
  /* if (error != NULL || trap != NULL) */
  /*   exit_with_error("failed to call function", error, trap); */

  printf("All finished!\n");
}
