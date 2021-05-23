#ifndef WASMTIME_HH
#define WASMTIME_HH

#include <any>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "wasmtime.h"

namespace wasmtime {

class Error {
  std::string msg;

public:
  Error(wasmtime_error_t *error) {
    wasm_byte_vec_t msg_bytes;
    wasmtime_error_message(error, &msg_bytes);
    msg = std::string(msg_bytes.data, msg_bytes.size);
    wasm_byte_vec_delete(&msg_bytes);
    wasmtime_error_delete(error);
  }

  std::string_view message() const { return msg; }
};

std::ostream &operator<<(std::ostream &os, const Error &e) {
  os << e.message();
  return os;
}

template <typename T, typename E = Error> class Result {
  std::variant<T, E> data;

public:
  Result(T t) : data(std::move(t)) {}
  Result(E e) : data(std::move(e)) {}

  operator bool() { return data.index() == 0; }

  E &&err() { return std::get<E>(std::move(data)); }
  const E &&err() const { return std::get<E>(std::move(data)); }
  T &&ok() { return std::get<T>(std::move(data)); }
  const T &&ok() const { return std::get<T>(std::move(data)); }
};

class Config {
  friend class Engine;

  struct deleter {
    void operator()(wasm_config_t *p) const { wasm_config_delete(p); }
  };

  std::unique_ptr<wasm_config_t, deleter> ptr;

public:
  Config() : ptr(wasm_config_new()) {}

  void debug_info(bool enable) {
    wasmtime_config_debug_info_set(ptr.get(), enable);
  }

  void interruptable(bool enable) {
    wasmtime_config_interruptable_set(ptr.get(), enable);
  }

  void consume_fuel(bool enable) {
    wasmtime_config_consume_fuel_set(ptr.get(), enable);
  }

  void max_wasm_stack(size_t stack) {
    wasmtime_config_max_wasm_stack_set(ptr.get(), stack);
  }

  void wasm_threads(bool enable) {
    wasmtime_config_wasm_threads_set(ptr.get(), enable);
  }

  void wasm_reference_types(bool enable) {
    wasmtime_config_wasm_reference_types_set(ptr.get(), enable);
  }

  void wasm_simd(bool enable) {
    wasmtime_config_wasm_simd_set(ptr.get(), enable);
  }

  void wasm_bulk_memory(bool enable) {
    wasmtime_config_wasm_bulk_memory_set(ptr.get(), enable);
  }

  void wasm_multi_value(bool enable) {
    wasmtime_config_wasm_multi_value_set(ptr.get(), enable);
  }

  void wasm_module_linking(bool enable) {
    wasmtime_config_wasm_module_linking_set(ptr.get(), enable);
  }

  enum Strategy {
    Auto = WASMTIME_STRATEGY_AUTO,
    Cranelift = WASMTIME_STRATEGY_CRANELIFT,
    Lightbeam = WASMTIME_STRATEGY_LIGHTBEAM,
  };

  [[nodiscard]] Result<std::monostate> strategy(Strategy strategy) {
    auto *error =
        wasmtime_config_strategy_set(ptr.get(), (wasmtime_strategy_t)strategy);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  void cranelift_debug_verifier(bool enable) {
    wasmtime_config_cranelift_debug_verifier_set(ptr.get(), enable);
  }

  enum OptLevel {
    OptNone = WASMTIME_OPT_LEVEL_NONE,
    OptSpeed = WASMTIME_OPT_LEVEL_SPEED,
    OptSpeedAndSize = WASMTIME_OPT_LEVEL_SPEED_AND_SIZE,
  };

  void cranelift_opt_level(OptLevel level) {
    wasmtime_config_cranelift_opt_level_set(ptr.get(),
                                            (wasmtime_opt_level_t)level);
  }

  enum ProfilingStrategy {
    ProfileNone = WASMTIME_PROFILING_STRATEGY_NONE,
    ProfileJitdump = WASMTIME_PROFILING_STRATEGY_JITDUMP,
    ProfileVtune = WASMTIME_PROFILING_STRATEGY_VTUNE,
  };

  [[nodiscard]] Result<std::monostate> profiler(ProfilingStrategy profiler) {
    auto *error = wasmtime_config_profiler_set(
        ptr.get(), (wasmtime_profiling_strategy_t)profiler);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  void static_memory_maximum_size(size_t size) {
    wasmtime_config_static_memory_maximum_size_set(ptr.get(), size);
  }

  void static_memory_guard_size(size_t size) {
    wasmtime_config_static_memory_guard_size_set(ptr.get(), size);
  }

  void dynamic_memory_guard_size(size_t size) {
    wasmtime_config_dynamic_memory_guard_size_set(ptr.get(), size);
  }

  [[nodiscard]] Result<std::monostate> cache_load_default() {
    auto *error = wasmtime_config_cache_config_load(ptr.get(), nullptr);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] Result<std::monostate> cache_load(std::string &path) {
    auto *error = wasmtime_config_cache_config_load(ptr.get(), path.c_str());
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }
};

class Engine {
  friend class Store;
  friend class Module;
  friend class Linker;

  struct deleter {
    void operator()(wasm_engine_t *p) const { wasm_engine_delete(p); }
  };

  std::unique_ptr<wasm_engine_t, deleter> ptr;

public:
  Engine() : ptr(wasm_engine_new()) {}
  Engine(Config config)
      : ptr(wasm_engine_new_with_config(config.ptr.release())) {}
};

[[nodiscard]] Result<std::vector<uint8_t>> wat2wasm(std::string_view wat) {
  wasm_byte_vec_t ret;
  auto *error = wasmtime_wat2wasm(wat.data(), wat.size(), &ret);
  if (error != nullptr) {
    return Error(error);
  }
  std::vector<uint8_t> vec;
  // NOLINTNEXTLINE TODO can this be done without triggering lints?
  std::span<uint8_t> raw(reinterpret_cast<uint8_t *>(ret.data), ret.size);
  vec.assign(raw.begin(), raw.end());
  wasm_byte_vec_delete(&ret);
  return vec;
}

class Limits {
  friend class MemoryType;
  friend class TableType;

  wasm_limits_t raw;

public:
  Limits(uint32_t min) : raw({.min = min, .max = wasm_limits_max_default}) {}
  Limits(uint32_t min, uint32_t max) : raw({.min = min, .max = max}) {}
  Limits(const wasm_limits_t *limits) : raw(*limits) {}

  uint32_t min() const { return raw.min; }

  std::optional<uint32_t> max() const {
    if (raw.max == wasm_limits_max_default) {
      return std::nullopt;
    }
    return raw.max;
  }
};

enum ValKind {
  KindI32,
  KindI64,
  KindF32,
  KindF64,
  KindV128,
  KindExternRef,
  KindFuncRef,
};

class ValType {
  friend class TableType;
  friend class GlobalType;
  friend class FuncType;

  struct deleter {
    void operator()(wasm_valtype_t *p) const { wasm_valtype_delete(p); }
  };

  std::unique_ptr<wasm_valtype_t, deleter> ptr;

  static wasm_valkind_t kind_to_c(ValKind kind) {
    switch (kind) {
    case KindI32:
      return WASM_I32;
    case KindI64:
      return WASM_I64;
    case KindF32:
      return WASM_F32;
    case KindF64:
      return WASM_F64;
    case KindExternRef:
      return WASM_ANYREF;
    case KindFuncRef:
      return WASM_FUNCREF;
    case KindV128:
      return WASMTIME_V128;
    }
    std::abort();
  }

public:
  class Ref {
    friend class ValType;

    const wasm_valtype_t *ptr;

  public:
    Ref(const wasm_valtype_t *ptr) : ptr(ptr) {}
    Ref(const ValType &ty) : Ref(ty.ptr.get()) {}

    ValKind kind() const {
      switch (wasm_valtype_kind(ptr)) {
      case WASM_I32:
        return KindI32;
      case WASM_I64:
        return KindI64;
      case WASM_F32:
        return KindF32;
      case WASM_F64:
        return KindF64;
      case WASM_ANYREF:
        return KindExternRef;
      case WASM_FUNCREF:
        return KindFuncRef;
      }
      std::abort();
    }
  };

  class ListRef {
    const wasm_valtype_vec_t *list;

  public:
    ListRef(const wasm_valtype_vec_t *list) : list(list) {}

    typedef const Ref *iterator;
    iterator begin() const {
      return reinterpret_cast<Ref *>(&list->data[0]); // NOLINT
    }
    iterator end() const {
      return reinterpret_cast<Ref *>(&list->data[list->size]); // NOLINT
    }
    size_t size() const { return list->size; }
  };

private:
  Ref ref;
  ValType(wasm_valtype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  ValType(ValKind kind) : ValType(wasm_valtype_new(kind_to_c(kind))) {}
  ValType(Ref other) : ValType(wasm_valtype_copy(other.ptr)) {}
  ValType(const ValType &other) : ValType(wasm_valtype_copy(other.ptr.get())) {}
  ValType &operator=(const ValType &other) {
    ptr.reset(wasm_valtype_copy(other.ptr.get()));
    return *this;
  }
  ~ValType() = default;
  ValType(ValType &&other) = default;
  ValType &operator=(ValType &&other) = default;

  static ValType i32() { return ValType(KindI32); }
  static ValType i64() { return ValType(KindI64); }
  static ValType f32() { return ValType(KindF32); }
  static ValType f64() { return ValType(KindF64); }
  static ValType externref() { return ValType(KindExternRef); }
  static ValType funcref() { return ValType(KindFuncRef); }

  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class MemoryType {
  friend class Memory;

  struct deleter {
    void operator()(wasm_memorytype_t *p) const { wasm_memorytype_delete(p); }
  };

  std::unique_ptr<wasm_memorytype_t, deleter> ptr;

public:
  class Ref {
    friend class MemoryType;

    const wasm_memorytype_t *ptr;

  public:
    Ref(const wasm_memorytype_t *ptr) : ptr(ptr) {}
    Ref(const MemoryType &ty) : Ref(ty.ptr.get()) {}

    Limits limits() const { return Limits(wasm_memorytype_limits(ptr)); }
  };

private:
  Ref ref;
  MemoryType(wasm_memorytype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  MemoryType(Limits limits) : MemoryType(wasm_memorytype_new(&limits.raw)) {}
  MemoryType(Ref other) : MemoryType(wasm_memorytype_copy(other.ptr)) {}
  MemoryType(const MemoryType &other)
      : MemoryType(wasm_memorytype_copy(other.ptr.get())) {}
  MemoryType &operator=(const MemoryType &other) {
    ptr.reset(wasm_memorytype_copy(other.ptr.get()));
    return *this;
  }
  ~MemoryType() = default;
  MemoryType(MemoryType &&other) = default;
  MemoryType &operator=(MemoryType &&other) = default;

  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class TableType {
  friend class Table;

  struct deleter {
    void operator()(wasm_tabletype_t *p) const { wasm_tabletype_delete(p); }
  };

  std::unique_ptr<wasm_tabletype_t, deleter> ptr;

public:
  class Ref {
    friend class TableType;

    const wasm_tabletype_t *ptr;

  public:
    Ref(const wasm_tabletype_t *ptr) : ptr(ptr) {}
    Ref(const TableType &ty) : Ref(ty.ptr.get()) {}

    Limits limits() const { return wasm_tabletype_limits(ptr); }

    ValType::Ref element() const { return wasm_tabletype_element(ptr); }
  };

private:
  Ref ref;
  TableType(wasm_tabletype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  TableType(ValType ty, Limits limits)
      : TableType(wasm_tabletype_new(ty.ptr.release(), &limits.raw)) {}
  TableType(Ref other) : TableType(wasm_tabletype_copy(other.ptr)) {}
  TableType(const TableType &other)
      : TableType(wasm_tabletype_copy(other.ptr.get())) {}
  TableType &operator=(const TableType &other) {
    ptr.reset(wasm_tabletype_copy(other.ptr.get()));
    return *this;
  }
  ~TableType() = default;
  TableType(TableType &&other) = default;
  TableType &operator=(TableType &&other) = default;

  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class GlobalType {
  friend class Global;

  struct deleter {
    void operator()(wasm_globaltype_t *p) const { wasm_globaltype_delete(p); }
  };

  std::unique_ptr<wasm_globaltype_t, deleter> ptr;

public:
  class Ref {
    friend class GlobalType;
    const wasm_globaltype_t *ptr;

  public:
    Ref(const wasm_globaltype_t *ptr) : ptr(ptr) {}
    Ref(const GlobalType &ty) : Ref(ty.ptr.get()) {}

    bool is_mutable() const {
      return wasm_globaltype_mutability(ptr) == WASM_VAR;
    }

    ValType::Ref content() const { return wasm_globaltype_content(ptr); }
  };

private:
  Ref ref;
  GlobalType(wasm_globaltype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  GlobalType(ValType ty, bool mut)
      : GlobalType(wasm_globaltype_new(
            ty.ptr.release(),
            (wasm_mutability_t)(mut ? WASM_VAR : WASM_CONST))) {}
  GlobalType(Ref other) : GlobalType(wasm_globaltype_copy(other.ptr)) {}
  GlobalType(const GlobalType &other)
      : GlobalType(wasm_globaltype_copy(other.ptr.get())) {}
  GlobalType &operator=(const GlobalType &other) {
    ptr.reset(wasm_globaltype_copy(other.ptr.get()));
    return *this;
  }
  ~GlobalType() = default;
  GlobalType(GlobalType &&other) = default;
  GlobalType &operator=(GlobalType &&other) = default;

  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class FuncType {
  friend class Func;

  struct deleter {
    void operator()(wasm_functype_t *p) const { wasm_functype_delete(p); }
  };

  std::unique_ptr<wasm_functype_t, deleter> ptr;

public:
  class Ref {
    friend class FuncType;
    const wasm_functype_t *ptr;

  public:
    Ref(const wasm_functype_t *ptr) : ptr(ptr) {}
    Ref(const FuncType &ty) : Ref(ty.ptr.get()) {}

    ValType::ListRef params() const { return wasm_functype_params(ptr); }

    ValType::ListRef results() const { return wasm_functype_results(ptr); }
  };

private:
  Ref ref;
  FuncType(wasm_functype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  FuncType(std::initializer_list<ValType> params,
           std::initializer_list<ValType> results)
      : ref(nullptr) {
    wasm_valtype_vec_t param_vec;
    wasm_valtype_vec_t result_vec;
    wasm_valtype_vec_new_uninitialized(&param_vec, params.size());
    wasm_valtype_vec_new_uninitialized(&result_vec, results.size());
    size_t i = 0;
    for (auto val : params) {
      param_vec.data[i++] = val.ptr.release(); // NOLINT
    }
    i = 0;
    for (auto val : results) {
      result_vec.data[i++] = val.ptr.release(); // NOLINT
    }

    ptr.reset(wasm_functype_new(&param_vec, &result_vec));
    ref = ptr.get();
  }
  FuncType(Ref other) : FuncType(wasm_functype_copy(other.ptr)) {}
  FuncType(const FuncType &other)
      : FuncType(wasm_functype_copy(other.ptr.get())) {}
  FuncType &operator=(const FuncType &other) {
    ptr.reset(wasm_functype_copy(other.ptr.get()));
    return *this;
  }
  ~FuncType() = default;
  FuncType(FuncType &&other) = default;
  FuncType &operator=(FuncType &&other) = default;

  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class ImportType {
public:
  class Ref {
    friend class TypeList;

    const wasm_importtype_t *ptr;

  public:
    Ref(const wasm_importtype_t *ptr) : ptr(ptr) {}

    std::string_view module() {
      const auto *name = wasm_importtype_module(ptr);
      return std::string_view(name->data, name->size);
    }

    std::string_view name() {
      const auto *name = wasm_importtype_name(ptr);
      return std::string_view(name->data, name->size);
    }

    // TODO: can this circle be broken another way?
    const wasm_externtype_t *raw_type() { return wasm_importtype_type(ptr); }
  };

  class List {
    friend class InstanceType;
    friend class ModuleType;

    wasm_importtype_vec_t list;

  public:
    List() : list({.size = 0, .data = nullptr}) {}
    List(const List &other) = delete;
    List(List &&other) noexcept : list(other.list) { other.list.size = 0; }
    ~List() {
      if (list.size > 0) {
        wasm_importtype_vec_delete(&list);
      }
    }

    List &operator=(const List &other) = delete;
    List &operator=(List &&other) noexcept {
      std::swap(list, other.list);
      return *this;
    }

    typedef const Ref *iterator;
    iterator begin() const {
      return reinterpret_cast<iterator>(&list.data[0]); // NOLINT
    }
    iterator end() const {
      return reinterpret_cast<iterator>(&list.data[list.size]); // NOLINT
    }
    size_t size() const { return list.size; }
  };
};

class ExportType {

public:
  class Ref {
    const wasm_exporttype_t *ptr;

  public:
    Ref(const wasm_exporttype_t *ptr) : ptr(ptr) {}

    std::string_view name() {
      const auto *name = wasm_exporttype_name(ptr);
      return std::string_view(name->data, name->size);
    }

    const wasm_externtype_t *raw_type() { return wasm_exporttype_type(ptr); }
  };

  class List {
    friend class InstanceType;
    friend class ModuleType;

    wasm_exporttype_vec_t list;

  public:
    List() : list({.size = 0, .data = nullptr}) {}
    List(const List &other) = delete;
    List(List &&other) noexcept : list(other.list) { other.list.size = 0; }
    ~List() {
      if (list.size > 0) {
        wasm_exporttype_vec_delete(&list);
      }
    }

    List &operator=(const List &other) = delete;
    List &operator=(List &&other) noexcept {
      std::swap(list, other.list);
      return *this;
    }

    typedef const Ref *iterator;
    iterator begin() const {
      return reinterpret_cast<iterator>(&list.data[0]); // NOLINT
    }
    iterator end() const {
      return reinterpret_cast<iterator>(&list.data[list.size]); // NOLINT
    }
    size_t size() const { return list.size; }
  };
};

class ModuleType {
  friend class Module;

  struct deleter {
    void operator()(wasmtime_moduletype_t *p) const {
      wasmtime_moduletype_delete(p);
    }
  };

  std::unique_ptr<wasmtime_moduletype_t, deleter> ptr;

public:
  class Ref {
    friend class ModuleType;

    const wasmtime_moduletype_t *ptr;

  public:
    Ref(const wasmtime_moduletype_t *ptr) : ptr(ptr) {}
    Ref(const ModuleType &ty) : Ref(ty.ptr.get()) {}

    ImportType::List imports() const {
      ImportType::List list;
      wasmtime_moduletype_imports(ptr, &list.list);
      return list;
    }

    ExportType::List exports() const {
      ExportType::List list;
      wasmtime_moduletype_exports(ptr, &list.list);
      return list;
    }
  };

private:
  Ref ref;
  ModuleType(wasmtime_moduletype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class InstanceType {
  friend class Instance;

  struct deleter {
    void operator()(wasmtime_instancetype_t *p) const {
      wasmtime_instancetype_delete(p);
    }
  };

  std::unique_ptr<wasmtime_instancetype_t, deleter> ptr;

public:
  class Ref {
    friend class InstanceType;

    const wasmtime_instancetype_t *ptr;

  public:
    Ref(const wasmtime_instancetype_t *ptr) : ptr(ptr) {}
    Ref(const InstanceType &ty) : Ref(ty.ptr.get()) {}

    ExportType::List exports() const {
      ExportType::List list;
      wasmtime_instancetype_exports(ptr, &list.list);
      return list;
    }
  };

private:
  Ref ref;
  InstanceType(wasmtime_instancetype_t *ptr) : ptr(ptr), ref(ptr) {}

public:
  Ref *operator->() { return &ref; }
  Ref *operator*() { return &ref; }
};

class ExternType {
  friend class ExportType;
  friend class ImportType;

public:
  typedef std::variant<FuncType::Ref, GlobalType::Ref, TableType::Ref,
                       MemoryType::Ref, ModuleType::Ref, InstanceType::Ref>
      Ref;

  // TODO: can this circle be broken another way?
  static Ref from_import(ImportType::Ref ty) {
    return ref_from_c(ty.raw_type());
  }

  // TODO: can this circle be broken another way?
  static Ref from_export(ExportType::Ref ty) {
    return ref_from_c(ty.raw_type());
  }

private:
  static Ref ref_from_c(const wasm_externtype_t *ptr) {
    switch (wasm_externtype_kind(ptr)) {
    case WASM_EXTERN_FUNC:
      return wasm_externtype_as_functype_const(ptr);
    case WASM_EXTERN_GLOBAL:
      return wasm_externtype_as_globaltype_const(ptr);
    case WASM_EXTERN_TABLE:
      return wasm_externtype_as_tabletype_const(ptr);
    case WASM_EXTERN_MEMORY:
      return wasm_externtype_as_memorytype_const(ptr);
    case WASMTIME_EXTERN_MODULE:
      // Should probably just add const versions of these functions to
      // wasmtime's C API?
      return wasmtime_externtype_as_moduletype(
          const_cast<wasm_externtype_t *>(ptr)); // NOLINT
    case WASMTIME_EXTERN_INSTANCE:
      return wasmtime_externtype_as_instancetype(
          const_cast<wasm_externtype_t *>(ptr)); // NOLINT
    }
    std::abort();
  }
};

class FrameRef {
  wasm_frame_t *frame;

public:
  uint32_t func_index() const { return wasm_frame_func_index(frame); }
  size_t func_offset() const { return wasm_frame_func_offset(frame); }
  size_t module_offset() const { return wasm_frame_module_offset(frame); }

  std::optional<std::string_view> func_name() const {
    const auto *name = wasmtime_frame_func_name(frame);
    if (name != nullptr) {
      return std::string_view(name->data, name->size);
    }
    return std::nullopt;
  }

  std::optional<std::string_view> module_name() const {
    const auto *name = wasmtime_frame_module_name(frame);
    if (name != nullptr) {
      return std::string_view(name->data, name->size);
    }
    return std::nullopt;
  }
};

class Trace {
  friend class Trap;

  wasm_frame_vec_t vec;

  Trace(wasm_frame_vec_t vec) : vec(vec) {}

public:
  ~Trace() { wasm_frame_vec_delete(&vec); }

  Trace(const Trace &other) = delete;
  Trace(Trace &&other) = delete;
  Trace &operator=(const Trace &other) = delete;
  Trace &operator=(Trace &&other) = delete;

  typedef const FrameRef *iterator;

  iterator begin() const {
    return reinterpret_cast<FrameRef *>(&vec.data[0]); // NOLINT
  }
  iterator end() const {
    return reinterpret_cast<FrameRef *>(&vec.data[vec.size]); // NOLINT
  }
  size_t size() const { return vec.size; }
};

class Trap {
  friend class Linker;
  friend class Instance;
  friend class Func;

  struct deleter {
    void operator()(wasm_trap_t *p) const { wasm_trap_delete(p); }
  };

  std::unique_ptr<wasm_trap_t, deleter> ptr;

  Trap(wasm_trap_t *ptr) : ptr(ptr) {}

public:
  Trap(std::string_view msg)
      : Trap(wasmtime_trap_new(msg.data(), msg.size())) {}

  std::string message() const {
    wasm_byte_vec_t msg;
    wasm_trap_message(ptr.get(), &msg);
    std::string ret(msg.data, msg.size);
    wasm_byte_vec_delete(&msg);
    return ret;
  }

  std::optional<int32_t> i32_exit() const {
    int32_t status = 0;
    if (wasmtime_trap_exit_status(ptr.get(), &status)) {
      return status;
    }
    return std::nullopt;
  }

  Trace trace() const {
    wasm_frame_vec_t frames;
    wasm_trap_trace(ptr.get(), &frames);
    return Trace(frames);
  }
};

struct TrapError {
  std::variant<Trap, Error> data;

  TrapError(Trap t) : data(std::move(t)) {}
  TrapError(Error e) : data(std::move(e)) {}

  std::string message() {
    if (auto *trap = std::get_if<Trap>(&data)) {
      return trap->message();
    }
    if (auto *error = std::get_if<Error>(&data)) {
      return std::string(error->message());
    }
    std::abort();
  }
};

template <typename T> using TrapResult = Result<T, TrapError>;

class Module {
  friend class Store;
  friend class Instance;
  friend class Linker;

  struct deleter {
    void operator()(wasmtime_module_t *p) const { wasmtime_module_delete(p); }
  };

  std::unique_ptr<wasmtime_module_t, deleter> ptr;

  Module(wasmtime_module_t *raw) : ptr(raw) {}

public:
  Module(const Module &other) : ptr(wasmtime_module_clone(other.ptr.get())) {}
  Module &operator=(const Module &other) {
    ptr.reset(wasmtime_module_clone(other.ptr.get()));
    return *this;
  }
  ~Module() = default;
  Module(Module &&other) = default;
  Module &operator=(Module &&other) = default;

  [[nodiscard]] static Result<Module> compile(Engine &engine,
                                              std::string_view wat) {
    auto wasm = wat2wasm(wat);
    if (!wasm) {
      return wasm.err();
    }
    auto bytes = wasm.ok();
    return compile(engine, bytes);
  }

  [[nodiscard]] static Result<Module> compile(Engine &engine,
                                              std::span<uint8_t> wasm) {
    wasmtime_module_t *ret = nullptr;
    auto *error =
        wasmtime_module_new(engine.ptr.get(), wasm.data(), wasm.size(), &ret);
    if (error != nullptr) {
      return Error(error);
    }
    return Module(ret);
  }

  [[nodiscard]] static Result<std::monostate>
  validate(Engine &engine, std::span<uint8_t> wasm) {
    auto *error =
        wasmtime_module_validate(engine.ptr.get(), wasm.data(), wasm.size());
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] static Result<Module> deserialize(Engine &engine,
                                                  std::span<uint8_t> wasm) {
    wasmtime_module_t *ret = nullptr;
    auto *error = wasmtime_module_deserialize(engine.ptr.get(), wasm.data(),
                                              wasm.size(), &ret);
    if (error != nullptr) {
      return Error(error);
    }
    return Module(ret);
  }

  ModuleType type() { return wasmtime_module_type(ptr.get()); }

  [[nodiscard]] Result<std::vector<uint8_t>> serialize() const {
    wasm_byte_vec_t bytes;
    auto *error = wasmtime_module_serialize(ptr.get(), &bytes);
    if (error != nullptr) {
      return Error(error);
    }
    std::vector<uint8_t> ret;
    // NOLINTNEXTLINE TODO can this be done without triggering lints?
    std::span<uint8_t> raw(reinterpret_cast<uint8_t *>(bytes.data), bytes.size);
    ret.assign(raw.begin(), raw.end());
    wasm_byte_vec_delete(&bytes);
    return ret;
  }
};

class InterruptHandle {
  friend class Store;

  struct deleter {
    void operator()(wasmtime_interrupt_handle_t *p) const {
      wasmtime_interrupt_handle_delete(p);
    }
  };

  std::unique_ptr<wasmtime_interrupt_handle_t, deleter> ptr;

  InterruptHandle(wasmtime_interrupt_handle_t *ptr) : ptr(ptr) {}

public:
  void interrupt() const { wasmtime_interrupt_handle_interrupt(ptr.get()); }
};

class WasiConfig {
  friend class Store;

  struct deleter {
    void operator()(wasi_config_t *p) const { wasi_config_delete(p); }
  };

  std::unique_ptr<wasi_config_t, deleter> ptr;

public:
  WasiConfig() : ptr(wasi_config_new()) {}

  void argv(const std::vector<std::string> &args) {
    std::vector<const char *> ptrs;
    ptrs.reserve(args.size());
    for (const auto &arg : args) {
      ptrs.push_back(arg.c_str());
    }

    wasi_config_set_argv(ptr.get(), (int)args.size(), ptrs.data());
  }

  void inherit_argv() { wasi_config_inherit_argv(ptr.get()); }

  void env(const std::vector<std::pair<std::string, std::string>> &env) {
    std::vector<const char *> names;
    std::vector<const char *> values;
    for (auto [name, value] : env) {
      names.push_back(name.c_str());
      values.push_back(value.c_str());
    }
    wasi_config_set_env(ptr.get(), (int)env.size(), names.data(),
                        values.data());
  }

  void inherit_env() { wasi_config_inherit_env(ptr.get()); }

  bool stdin_file(std::string &path) {
    return wasi_config_set_stdin_file(ptr.get(), path.c_str());
  }

  void inherit_stdin() { return wasi_config_inherit_stdin(ptr.get()); }

  bool stdout_file(std::string &path) {
    return wasi_config_set_stdout_file(ptr.get(), path.c_str());
  }

  void inherit_stdout() { return wasi_config_inherit_stdout(ptr.get()); }

  bool stderr_file(std::string &path) {
    return wasi_config_set_stderr_file(ptr.get(), path.c_str());
  }

  void inherit_stderr() { return wasi_config_inherit_stderr(ptr.get()); }

  bool preopen_dir(std::string &path, std::string &guest_path) {
    return wasi_config_preopen_dir(ptr.get(), path.c_str(), guest_path.c_str());
  }
};

class Store {
  struct deleter {
    void operator()(wasmtime_store_t *p) const { wasmtime_store_delete(p); }
  };

  std::unique_ptr<wasmtime_store_t, deleter> ptr;

public:
  Store(Engine &engine)
      : ptr(wasmtime_store_new(engine.ptr.get(), nullptr, nullptr)) {}

  class Context {
    friend class Global;
    friend class Table;
    friend class Memory;
    friend class Func;
    friend class Instance;
    friend class Linker;
    wasmtime_context_t *ptr;

    Context(wasmtime_context_t *ptr) : ptr(ptr) {}

  public:
    Context(Store &store) : Context(wasmtime_store_context(store.ptr.get())) {}
    Context(Store *store) : Context(wasmtime_store_context(store->ptr.get())) {}

    void gc() { wasmtime_context_gc(ptr); }

    [[nodiscard]] Result<std::monostate> add_fuel(uint64_t fuel) {
      auto *error = wasmtime_context_add_fuel(ptr, fuel);
      if (error != nullptr) {
        return Error(error);
      }
      return std::monostate();
    }

    std::optional<uint64_t> fuel_consumed() const {
      uint64_t fuel = 0;
      if (wasmtime_context_fuel_consumed(ptr, &fuel)) {
        return fuel;
      }
      return std::nullopt;
    }

    [[nodiscard]] Result<std::monostate> set_wasi(WasiConfig config) {
      auto *error = wasmtime_context_set_wasi(ptr, config.ptr.release());
      if (error != nullptr) {
        return Error(error);
      }
      return std::monostate();
    }

    std::optional<InterruptHandle> interrupt_handle() {
      auto *handle = wasmtime_interrupt_handle_new(ptr);
      if (handle != nullptr) {
        return InterruptHandle(handle);
      }
      return std::nullopt;
    }
  };

  Context context() { return this; }
};

class ExternRef {
  friend class Val;

  struct deleter {
    void operator()(wasmtime_externref_t *p) const {
      wasmtime_externref_delete(p);
    }
  };

  std::unique_ptr<wasmtime_externref_t, deleter> ptr;

  static void finalizer(void *ptr) {
    std::unique_ptr<std::any> _ptr(static_cast<std::any *>(ptr));
  }

  ExternRef(wasmtime_externref_t *ptr) : ptr(ptr) {}

public:
  ExternRef(std::any val)
      : ExternRef(wasmtime_externref_new(
            std::make_unique<std::any>(std::move(val)).release(), finalizer)) {}
  ExternRef(const ExternRef &other)
      : ExternRef(wasmtime_externref_clone(other.ptr.get())) {}
  ExternRef &operator=(const ExternRef &other) {
    ptr.reset(wasmtime_externref_clone(other.ptr.get()));
    return *this;
  }
  ExternRef(ExternRef &&other) = default;
  ExternRef &operator=(ExternRef &&other) = default;
  ~ExternRef() = default;

  std::any &data() {
    return *static_cast<std::any *>(wasmtime_externref_data(ptr.get()));
  }
};

class Func;

class Val {
  friend class Global;
  friend class Table;
  friend class Func;

  wasmtime_val_t val;

  Val() : val({.kind = WASMTIME_I32, .of = {.i32 = 0}}) {}
  Val(wasmtime_val_t val) : val(val) {}

public:
  Val(int32_t i32) : val({.kind = WASMTIME_I32, .of = {.i32 = i32}}) {}
  Val(int64_t i64) : val({.kind = WASMTIME_I64, .of = {.i64 = i64}}) {}
  Val(float f32) : val({.kind = WASMTIME_F32, .of = {.f32 = f32}}) {}
  Val(double f64) : val({.kind = WASMTIME_F64, .of = {.f64 = f64}}) {}
  Val(wasmtime_v128 v128) : val({.kind = WASMTIME_V128, .of = {.i32 = 0}}) {
    memcpy(&val.of.v128[0], v128, sizeof(wasmtime_v128));
  }
  Val(std::optional<Func *> func);
  Val(ExternRef ptr)
      : val({.kind = WASMTIME_EXTERNREF,
             .of = {.externref = ptr.ptr.release()}}) {}
  Val(std::optional<ExternRef> ptr)
      : val({.kind = WASMTIME_EXTERNREF, .of = {.externref = nullptr}}) {
    if (ptr) {
      val.of.externref = ptr->ptr.release();
    } else {
      val.of.externref = nullptr;
    }
  }
  Val(const Val &other) : val({.kind = WASMTIME_I32, .of = {.i32 = 0}}) {
    wasmtime_val_copy(&val, &other.val);
  }
  Val(Val &&other) noexcept : val({.kind = WASMTIME_I32, .of = {.i32 = 0}}) {
    std::swap(val, other.val);
  }

  ~Val() { wasmtime_val_delete(&val); }

  Val &operator=(const Val &other) = delete;
  Val &operator=(Val &&other) = delete;

  ValKind Kind() const {
    switch (val.kind) {
    case WASMTIME_I32:
      return KindI32;
    case WASMTIME_I64:
      return KindI64;
    case WASMTIME_F32:
      return KindF32;
    case WASMTIME_F64:
      return KindF64;
    case WASMTIME_FUNCREF:
      return KindFuncRef;
    case WASMTIME_EXTERNREF:
      return KindExternRef;
    case WASMTIME_V128:
      return KindV128;
    }
    std::abort();
  }

  int32_t i32() const {
    if (val.kind != WASMTIME_I32) {
      std::abort();
    }
    return val.of.i32;
  }

  int64_t i64() const {
    if (val.kind != WASMTIME_I64) {
      std::abort();
    }
    return val.of.i64;
  }

  float f32() const {
    if (val.kind != WASMTIME_F32) {
      std::abort();
    }
    return val.of.f32;
  }

  double f64() const {
    if (val.kind != WASMTIME_F64) {
      std::abort();
    }
    return val.of.f64;
  }

  const uint8_t *v128() const {
    if (val.kind != WASMTIME_V128) {
      std::abort();
    }
    return &val.of.v128[0];
  }

  std::optional<ExternRef> externref() const {
    if (val.kind != WASMTIME_EXTERNREF) {
      std::abort();
    }
    if (val.of.externref != nullptr) {
      return ExternRef(wasmtime_externref_clone(val.of.externref));
    }
    return std::nullopt;
  }

  std::optional<Func> func() const;
};

class Global {
  friend class Instance;
  wasmtime_global_t global;

public:
  Global(wasmtime_global_t global) : global(global) {}

  [[nodiscard]] static Result<Global>
  create(Store::Context cx, const GlobalType &ty, const Val &init) {
    wasmtime_global_t global;
    auto *error = wasmtime_global_new(cx.ptr, ty.ptr.get(), &init.val, &global);
    if (error != nullptr) {
      return Error(error);
    }
    return Global(global);
  }

  GlobalType type(Store::Context cx) const {
    return wasmtime_global_type(cx.ptr, &global);
  }

  Val get(Store::Context cx) const {
    Val val;
    wasmtime_global_get(cx.ptr, &global, &val.val);
    return val;
  }

  [[nodiscard]] Result<std::monostate> set(Store::Context cx,
                                           const Val &val) const {
    auto *error = wasmtime_global_set(cx.ptr, &global, &val.val);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }
};

class Table {
  friend class Instance;
  wasmtime_table_t table;

public:
  Table(wasmtime_table_t table) : table(table) {}

  [[nodiscard]] static Result<Table>
  create(Store::Context cx, const TableType &ty, const Val &init) {
    wasmtime_table_t table;
    auto *error = wasmtime_table_new(cx.ptr, ty.ptr.get(), &init.val, &table);
    if (error != nullptr) {
      return Error(error);
    }
    return Table(table);
  }

  TableType type(Store::Context cx) const {
    return wasmtime_table_type(cx.ptr, &table);
  }

  size_t size(Store::Context cx) const {
    return wasmtime_table_size(cx.ptr, &table);
  }

  std::optional<Val> get(Store::Context cx, uint32_t idx) const {
    Val val;
    if (wasmtime_table_get(cx.ptr, &table, idx, &val.val)) {
      return val;
    }
    return std::nullopt;
  }

  [[nodiscard]] Result<std::monostate> set(Store::Context cx, uint32_t idx,
                                           const Val &val) const {
    auto *error = wasmtime_table_set(cx.ptr, &table, idx, &val.val);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] Result<uint32_t> grow(Store::Context cx, uint32_t delta,
                                      const Val &init) const {
    uint32_t prev = 0;
    auto *error = wasmtime_table_grow(cx.ptr, &table, delta, &init.val, &prev);
    if (error != nullptr) {
      return Error(error);
    }
    return prev;
  }
};

class Memory {
  friend class Instance;
  wasmtime_memory_t memory;

public:
  Memory(wasmtime_memory_t memory) : memory(memory) {}

  [[nodiscard]] static Result<Memory> create(Store::Context cx,
                                             const MemoryType &ty) {
    wasmtime_memory_t memory;
    auto *error = wasmtime_memory_new(cx.ptr, ty.ptr.get(), &memory);
    if (error != nullptr) {
      return Error(error);
    }
    return Memory(memory);
  }

  MemoryType type(Store::Context cx) const {
    return wasmtime_memory_type(cx.ptr, &memory);
  }

  uint32_t size(Store::Context cx) const {
    return wasmtime_memory_size(cx.ptr, &memory);
  }

  std::span<uint8_t> data(Store::Context cx) const {
    auto *base = wasmtime_memory_data(cx.ptr, &memory);
    auto size = wasmtime_memory_data_size(cx.ptr, &memory);
    return {base, size};
  }

  [[nodiscard]] Result<uint32_t> grow(Store::Context cx, uint32_t delta) const {
    uint32_t prev = 0;
    auto *error = wasmtime_memory_grow(cx.ptr, &memory, delta, &prev);
    if (error != nullptr) {
      return Error(error);
    }
    return prev;
  }
};

class Func {
  friend class Val;
  friend class Instance;

  wasmtime_func_t func;

public:
  Func(wasmtime_func_t func) : func(func) {}

  // TODO
  /* static Result<Func> create(Store::Context cx, const FuncType &ty) { */
  /*   wasmtime_func_t func; */
  /*   auto *error = wasmtime_func_new(cx.ptr, ty.ptr.get(), &func); */
  /*   if (error != nullptr) */
  /*     return Error(error); */
  /*   return Func(func); */
  /* } */

  [[nodiscard]] TrapResult<std::vector<Val>>
  call(Store::Context cx, const std::vector<Val> &params) {
    std::vector<wasmtime_val_t> raw_params;
    raw_params.reserve(params.size());
    for (const auto &param : params) {
      raw_params.push_back(param.val);
    }
    size_t nresults = this->type(cx)->results().size();
    std::vector<wasmtime_val_t> raw_results(nresults);

    wasm_trap_t *trap = nullptr;
    auto *error =
        wasmtime_func_call(cx.ptr, &func, raw_params.data(), raw_params.size(),
                           raw_results.data(), raw_results.capacity(), &trap);
    if (error != nullptr) {
      return TrapError(Error(error));
    }
    if (trap != nullptr) {
      return TrapError(Trap(trap));
    }

    std::vector<Val> results;
    results.reserve(nresults);
    for (size_t i = 0; i < nresults; i++) {
      results.push_back(raw_results[i]);
    }
    return results;
  }

  FuncType type(Store::Context cx) const {
    return wasmtime_func_type(cx.ptr, &func);
  }
};

Val::Val(std::optional<Func *> func)
    : val({.kind = WASMTIME_FUNCREF, .of = {.i32 = 0}}) {
  if (!func) {
    return;
  }
  val.of.funcref = (**func).func;
}

std::optional<Func> Val::func() const {
  if (val.kind != WASMTIME_FUNCREF) {
    std::abort();
  }
  if (val.of.funcref.store_id == 0) {
    return std::nullopt;
  }
  return Func(val.of.funcref);
}

class Instance;

// TODO: ExternType?
typedef std::variant<Instance, Module, Func, Global, Memory, Table> Extern;

class Instance {
  friend class Linker;

  wasmtime_instance_t instance;

  static Extern cvt(wasmtime_extern_t &e) {
    switch (e.kind) {
    case WASMTIME_EXTERN_FUNC:
      return Func(e.of.func);
    case WASMTIME_EXTERN_GLOBAL:
      return Global(e.of.global);
    case WASMTIME_EXTERN_MEMORY:
      return Memory(e.of.memory);
    case WASMTIME_EXTERN_TABLE:
      return Table(e.of.table);
    case WASMTIME_EXTERN_INSTANCE:
      return Instance(e.of.instance);
    case WASMTIME_EXTERN_MODULE:
      return Module(e.of.module);
    }
    std::abort();
  }

  static void cvt(Extern &e, wasmtime_extern_t &raw) {
    if (auto *func = std::get_if<Func>(&e)) {
      raw.kind = WASMTIME_EXTERN_FUNC;
      raw.of.func = func->func;
    } else if (auto *global = std::get_if<Global>(&e)) {
      raw.kind = WASMTIME_EXTERN_GLOBAL;
      raw.of.global = global->global;
    } else if (auto *table = std::get_if<Table>(&e)) {
      raw.kind = WASMTIME_EXTERN_TABLE;
      raw.of.table = table->table;
    } else if (auto *memory = std::get_if<Memory>(&e)) {
      raw.kind = WASMTIME_EXTERN_MEMORY;
      raw.of.memory = memory->memory;
    } else if (auto *instance = std::get_if<Instance>(&e)) {
      raw.kind = WASMTIME_EXTERN_INSTANCE;
      raw.of.instance = instance->instance;
    } else if (auto *module = std::get_if<Module>(&e)) {
      raw.kind = WASMTIME_EXTERN_MODULE;
      raw.of.module = module->ptr.get();
    } else {
      std::abort();
    }
  }

public:
  Instance(wasmtime_instance_t instance) : instance(instance) {}

  [[nodiscard]] static TrapResult<Instance>
  create(Store::Context cx, const Module &m,
         const std::vector<Extern> &imports) {
    std::vector<wasmtime_extern_t> raw_imports;
    for (auto item : imports) {
      raw_imports.push_back(wasmtime_extern_t{});
      auto last = raw_imports.back();
      Instance::cvt(item, last);
    }
    wasmtime_instance_t instance;
    wasm_trap_t *trap = nullptr;
    auto *error = wasmtime_instance_new(cx.ptr, m.ptr.get(), raw_imports.data(),
                                        raw_imports.size(), &instance, &trap);
    if (error != nullptr) {
      return TrapError(Error(error));
    }
    if (trap != nullptr) {
      return TrapError(Trap(trap));
    }
    return Instance(instance);
  }

  InstanceType type(Store::Context cx) const {
    return wasmtime_instance_type(cx.ptr, &instance);
  }

  std::optional<Extern> get(Store::Context cx, std::string_view name) {
    wasmtime_extern_t e;
    if (!wasmtime_instance_export_get(cx.ptr, &instance, name.data(),
                                      name.size(), &e)) {
      return std::nullopt;
    }
    return Instance::cvt(e);
  }

  std::optional<std::pair<std::string_view, Extern>> get(Store::Context cx,
                                                         size_t idx) {
    wasmtime_extern_t e;
    // I'm not sure why clang-tidy thinks this is using va_list or anything
    // related to that...
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    char *name = nullptr;
    size_t len = 0;
    if (!wasmtime_instance_export_nth(cx.ptr, &instance, idx, &name, &len,
                                      &e)) {
      return std::nullopt;
    }
    std::string_view n(name, len);
    return std::pair(n, Instance::cvt(e));
  }
};

class Linker {
  struct deleter {
    void operator()(wasmtime_linker_t *p) const { wasmtime_linker_delete(p); }
  };

  std::unique_ptr<wasmtime_linker_t, deleter> ptr;

public:
  Linker(Engine &engine) : ptr(wasmtime_linker_new(engine.ptr.get())) {}

  void allow_shadowing(bool allow) {
    wasmtime_linker_allow_shadowing(ptr.get(), allow);
  }

  [[nodiscard]] Result<std::monostate>
  define(std::string_view module, std::string_view name, Extern &item) {
    wasmtime_extern_t raw;
    Instance::cvt(item, raw);
    auto *error =
        wasmtime_linker_define(ptr.get(), module.data(), module.size(),
                               name.data(), name.size(), &raw);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] Result<std::monostate> define_wasi() {
    auto *error = wasmtime_linker_define_wasi(ptr.get());
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] Result<std::monostate>
  define_instance(Store::Context cx, std::string_view name, Instance instance) {
    auto *error = wasmtime_linker_define_instance(
        ptr.get(), cx.ptr, name.data(), name.size(), &instance.instance);
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] TrapResult<Instance> instantiate(Store::Context cx,
                                                 const Module &m) {
    wasmtime_instance_t instance;
    wasm_trap_t *trap = nullptr;
    auto *error = wasmtime_linker_instantiate(ptr.get(), cx.ptr, m.ptr.get(),
                                              &instance, &trap);
    if (error != nullptr) {
      return TrapError(Error(error));
    }
    if (trap != nullptr) {
      return TrapError(Trap(trap));
    }
    return Instance(instance);
  }

  [[nodiscard]] Result<std::monostate>
  module(Store::Context cx, std::string_view name, const Module &m) {
    auto *error = wasmtime_linker_module(ptr.get(), cx.ptr, name.data(),
                                         name.size(), m.ptr.get());
    if (error != nullptr) {
      return Error(error);
    }
    return std::monostate();
  }

  [[nodiscard]] std::optional<Extern>
  get(Store::Context cx, std::string_view module, std::string_view name) {
    wasmtime_extern_t item;
    if (wasmtime_linker_get(ptr.get(), cx.ptr, module.data(), module.size(),
                            name.data(), name.size(), &item)) {
      return Instance::cvt(item);
    }
    return std::nullopt;
  }

  [[nodiscard]] Result<Func> get_default(Store::Context cx,
                                         std::string_view name) {
    wasmtime_func_t item;
    auto *error = wasmtime_linker_get_default(ptr.get(), cx.ptr, name.data(),
                                              name.size(), &item);
    if (error != nullptr) {
      return Error(error);
    }
    return Func(item);
  }
};

} // namespace wasmtime

#endif // WASMTIME_HH
