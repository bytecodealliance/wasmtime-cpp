include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/HEAD.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

function(add_test name)

  add_executable(test-${name} ${name}.cc)
  target_link_libraries(test-${name} gtest_main)
  configure_wasmtime_target(test-${name})

  gtest_discover_tests(test-${name})
endfunction()

add_test(simple)
add_test(types)