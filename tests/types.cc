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

TEST(Limits, Smoke) {
  Limits limits(1);
  EXPECT_EQ(limits.min(), 1);
  EXPECT_EQ(limits.max(), std::nullopt);

  limits = Limits(2, 3);
  EXPECT_EQ(limits.min(), 2);
  EXPECT_EQ(limits.max(), 3);
}

TEST(ValType, Smoke) {
  EXPECT_EQ(ValType::i32()->kind(), KindI32);
  EXPECT_EQ(ValType::i64()->kind(), KindI64);
  EXPECT_EQ(ValType::f32()->kind(), KindF32);
  EXPECT_EQ(ValType::f64()->kind(), KindF64);
  EXPECT_EQ(ValType::externref()->kind(), KindExternRef);
  EXPECT_EQ(ValType::funcref()->kind(), KindFuncRef);
  EXPECT_EQ(ValType::v128()->kind(), KindV128);

  ValType t(KindI32);
  t = KindI64;
  ValType t2(KindF32);
  t = t2;
  ValType t3(t2);

  ValType t4(**t);
}

TEST(MemoryType, Smoke) {
  MemoryType t(Limits(1));

  EXPECT_EQ(t->limits().min(), 1);
  EXPECT_EQ(t->limits().max(), std::nullopt);
}

TEST(TableType, Smoke) {
  TableType t(KindFuncRef, Limits(1));

  EXPECT_EQ(t->limits().min(), 1);
  EXPECT_EQ(t->limits().max(), std::nullopt);
  EXPECT_EQ(t->element().kind(), KindFuncRef);
}

TEST(GlobalType, Smoke) {
  GlobalType t(KindFuncRef, true);

  EXPECT_EQ(t->content().kind(), KindFuncRef);
  EXPECT_TRUE(t->is_mutable());
}

TEST(FuncType, Smoke) {
  FuncType t({}, {});
  EXPECT_EQ(t->params().size(), 0);
  EXPECT_EQ(t->results().size(), 0);

  FuncType t2({KindI32}, {KindI64});
  EXPECT_EQ(t2->params().size(), 1);
  for (auto ty : t2->params()) {
    EXPECT_EQ(ty.kind(), KindI32);
  }
  EXPECT_EQ(t2->results().size(), 1);
  for (auto ty : t2->results()) {
    EXPECT_EQ(ty.kind(), KindI64);
  }
}

TEST(ModuleType, Smoke) {
  Engine engine;
  Module module = unwrap(Module::compile(engine, "(module)"));
  ModuleType ty = module.type();
  EXPECT_EQ(ty->imports().size(), 0);
  EXPECT_EQ(ty->exports().size(), 0);

  module = unwrap(Module::compile(engine,
    "(module"
      "(import \"a\" \"b\" (func))"
      "(global (export \"x\") i32 (i32.const 0))"
    ")"
  ));
  ty = module.type();

  auto imports = ty->imports();
  EXPECT_EQ(imports.size(), 1);
  auto i = *imports.begin();
  EXPECT_EQ(i.module(), "a");
  EXPECT_EQ(i.name(), "b");
  auto import_ty = std::get<FuncType::Ref>(ExternType::from_import(i));
  EXPECT_EQ(import_ty.params().size(), 0);
  EXPECT_EQ(import_ty.results().size(), 0);

  auto exports = ty->exports();
  EXPECT_EQ(exports.size(), 1);
  auto e = *exports.begin();
  EXPECT_EQ(e.name(), "x");
  auto export_ty = std::get<GlobalType::Ref>(ExternType::from_export(e));
  EXPECT_EQ(export_ty.content().kind(), KindI32);
  EXPECT_FALSE(export_ty.is_mutable());
}

TEST(InstanceType, Smoke) {
  Engine engine;
  Module module = unwrap(Module::compile(engine, "(module)"));
  Store store(engine);
  Instance instance = unwrap(Instance::create(store, module, {}));
  auto ty = instance.type(store);
  EXPECT_EQ(ty->exports().size(), 0);

  module = unwrap(Module::compile(engine,
    "(module"
      "(global (export \"x\") i32 (i32.const 0))"
    ")"
  ));
  instance = unwrap(Instance::create(store, module, {}));
  ty = instance.type(store);

  auto exports = ty->exports();
  EXPECT_EQ(exports.size(), 1);
  auto e = *exports.begin();
  EXPECT_EQ(e.name(), "x");
  auto export_ty = std::get<GlobalType::Ref>(ExternType::from_export(e));
  EXPECT_EQ(export_ty.content().kind(), KindI32);
  EXPECT_FALSE(export_ty.is_mutable());
}
