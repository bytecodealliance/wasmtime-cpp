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

  store = Store(engine);
  store.context().gc();
  EXPECT_EQ(store.context().fuel_consumed(), std::nullopt);
  EXPECT_EQ(store.context().interrupt_handle(), std::nullopt);
  store.context().add_fuel(1).err();
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

TEST(Trap, Smoke) {
  Trap t("foo");
  EXPECT_EQ(t.message(), "foo");
  EXPECT_EQ(t.i32_exit(), std::nullopt);
  EXPECT_EQ(t.trace().size(), 0);

  Engine engine;
  Module m = unwrap(Module::compile(engine, "(module (func (export \"\") unreachable))"));
  Store store(engine);
  Instance i = unwrap(Instance::create(store, m, {}));
  auto func = std::get<Func>(*i.get(store, ""));
  auto trap = std::get<Trap>(func.call(store, {}).err().data);
  auto trace = trap.trace();
  EXPECT_EQ(trace.size(), 1);
  auto frame = *trace.begin();
  EXPECT_EQ(frame.func_name(), std::nullopt);
  EXPECT_EQ(frame.module_name(), std::nullopt);
  EXPECT_EQ(frame.func_index(), 0);
  EXPECT_EQ(frame.func_offset(), 1);
  EXPECT_EQ(frame.module_offset(), 29);
}

TEST(Module, Smoke) {
  Engine engine;
  Module::compile(engine, "(module)").ok();
  Module::compile(engine, "wat").err();

  auto wasm = wat2wasm("(module)").ok();
  Module::compile(engine, wasm).ok();
  std::vector<uint8_t> emptyWasm;
  Module::compile(engine, emptyWasm).err();

  Module::validate(engine, wasm).ok();
  Module::validate(engine, emptyWasm).err();
}

TEST(Module, Serialize) {
  Engine engine;
  Module m = unwrap(Module::compile(engine, "(module)"));
  auto bytes = unwrap(m.serialize());
  m = unwrap(Module::deserialize(engine, bytes));
}

TEST(WasiConfig, Smoke) {
  WasiConfig config;
  config.argv({"x"});
  config.inherit_argv();
  config.env({{"x", "y"}});
  config.inherit_env();
  EXPECT_FALSE(config.stdin_file("nonexistent"));
  config.inherit_stdin();
  EXPECT_FALSE(config.stdout_file("path/to/nonexistent"));
  config.inherit_stdout();
  EXPECT_FALSE(config.stderr_file("path/to/nonexistent"));
  config.inherit_stderr();
  EXPECT_FALSE(config.preopen_dir("nonexistent", "nonexistent"));
}

TEST(ExternRef, Smoke) {
  ExternRef a("foo");
  ExternRef b(3);
  EXPECT_STREQ(std::any_cast<const char*>(a.data()), "foo");
  EXPECT_EQ(std::any_cast<int>(b.data()), 3);
}

TEST(Val, Smoke) {
  Val val(1);
  EXPECT_EQ(val.kind(), KindI32);
  EXPECT_EQ(val.i32(), 1);

  val = (int32_t) 3;
  EXPECT_EQ(val.kind(), KindI32);
  EXPECT_EQ(val.i32(), 3);

  val = (int64_t) 4;
  EXPECT_EQ(val.kind(), KindI64);
  EXPECT_EQ(val.i64(), 4);

  val = (float) 5;
  EXPECT_EQ(val.kind(), KindF32);
  EXPECT_EQ(val.f32(), 5);

  val = (double) 6;
  EXPECT_EQ(val.kind(), KindF64);
  EXPECT_EQ(val.f64(), 6);

  wasmtime_v128 v128 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  val = v128;
  EXPECT_EQ(val.kind(), KindV128);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(val.v128()[i], i);
  }

  val = std::optional<ExternRef>(std::nullopt);
  EXPECT_EQ(val.kind(), KindExternRef);
  EXPECT_EQ(val.externref(), std::nullopt);

  val = std::optional<ExternRef>(5);
  EXPECT_EQ(val.kind(), KindExternRef);
  EXPECT_EQ(std::any_cast<int>(val.externref()->data()), 5);

  val = ExternRef(5);
  EXPECT_EQ(val.kind(), KindExternRef);
  EXPECT_EQ(std::any_cast<int>(val.externref()->data()), 5);

  val = std::optional<Func*>(std::nullopt);
  EXPECT_EQ(val.kind(), KindFuncRef);
  EXPECT_EQ(val.funcref(), std::nullopt);

  Engine engine;
  Store store(engine);
  Func func(store, FuncType({}, {}), [](auto caller, auto params, auto results) -> auto {
    return std::monostate();
  });

  val = std::optional<Func*>(&func);
  EXPECT_EQ(val.kind(), KindFuncRef);

  val = &func;
  EXPECT_EQ(val.kind(), KindFuncRef);
}
