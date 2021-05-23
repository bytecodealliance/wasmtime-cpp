cmake_minimum_required(VERSION 3.10)
project(Wasmtime VERSION 0.27.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_CODE_ANALYSIS "Run code analysis" OFF)
message(STATUS "ENABLE_CODE_ANALYSIS       ${ENABLE_CODE_ANALYSIS}")

if(ENABLE_CODE_ANALYSIS)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_CLANG_TIDY clang-tidy
        -checks=-*,readability-*,bugprone-*,-bugprone-exception-escape,clang-analyzer-*,cppcoreguidelines-*,portability-*,performance-*
        -header-filter=wasmtime.hh
        -warnings-as-errors=*)
  endif()
endif()

function(configure_wasmtime_target name)
  if (MSVC)
    target_compile_options(${name} PUBLIC /W4 /WX /DWASM_API_EXTERN= /DWASI_API_EXTERN=)
    target_link_libraries(${name} ws2_32 bcrypt advapi32 userenv ntdll shell32 ole32)
  else()
    target_compile_options(${name} PUBLIC -Wall -Wextra -pedantic -Werror -Wno-unused-command-line-argument)
  endif()

  target_link_libraries(${name} wasmtime)
  if (UNIX)
    target_link_libraries(${name} stdc++ pthread)
  endif (UNIX)
  target_include_directories(${name} PUBLIC
      ${PROJECT_SOURCE_DIR}/include
      ${PROJECT_SOURCE_DIR}/c-api/include)
  target_link_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR}/c-api/lib)
endfunction()


enable_testing()
add_subdirectory(examples)

add_executable(test-all tests/all.cc)
add_test(NAME smoke COMMAND test-all)
configure_wasmtime_target(test-all)
