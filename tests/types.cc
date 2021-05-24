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
