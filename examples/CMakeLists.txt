function(add_example name)
  add_executable(${name} ${name}.cc)
  target_link_libraries(${name} PRIVATE wasmtime-cpp)
  if (MSVC)
    target_compile_options(${name} INTERFACE /W4 /WX)
  else()
    target_compile_options(${name} INTERFACE -Wall -Wextra -pedantic -Werror -Wno-unused-command-line-argument)
  endif()
  add_test(
    NAME example-${name}
    COMMAND ${name}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

  if(ENABLE_CODE_ANALYSIS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set_target_properties(${name} PROPERTIES
        CXX_CLANG_TIDY
        "clang-tidy;-checks=-*,readability-*,bugprone-*,-bugprone-exception-escape,clang-analyzer-*,cppcoreguidelines-*,portability-*,performance-*;-header-filter=wasmtime.hh;-warnings-as-errors=*")
    endif()
  endif()
endfunction()

add_example(fuel)
add_example(interrupt)
add_example(linking)
add_example(externref)
add_example(gcd)
add_example(memory)
add_example(hello)

# These targets give warnings about constants and such we don't really care
# about or want to fix.
set_target_properties(memory PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(gcd PROPERTIES CXX_CLANG_TIDY "")
