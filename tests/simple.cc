#include <gtest/gtest.h>
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

TEST(Store, Smoke) {
  Engine engine;
  Store store(engine);
  Store store2 = std::move(store);
  Store store3(std::move(store2));
}

TEST(Engine, Smoke) {
  Engine engine;
  Config config;
  engine = Engine(std::move(config));
}

TEST(Config, Smoke) {
  Config config;
  config.debug_info(false);
  config.interruptable(false);
  config.consume_fuel(false);
  config.max_wasm_stack(100);
  config.wasm_threads(false);
  config.wasm_reference_types(false);
  config.wasm_simd(false);
  config.wasm_bulk_memory(false);
  config.wasm_multi_value(false);
  config.wasm_module_linking(false);
  unwrap(config.strategy(Config::Auto));
  config.cranelift_debug_verifier(false);
  config.cranelift_opt_level(Config::OptSpeed);
  unwrap(config.profiler(Config::ProfileNone));
  config.static_memory_maximum_size(0);
  config.static_memory_guard_size(0);
  auto result = config.cache_load_default();
  config.cache_load("nonexistent").err();

  Config config2 = std::move(config);
  Config config3(std::move(config));
}

TEST(wat2wasm, Smoke) {
  wat2wasm("(module)").ok();
  wat2wasm("xxx").err();
}
