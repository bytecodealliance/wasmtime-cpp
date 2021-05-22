cmake_minimum_required(VERSION 3.10)
project(Wasmtime VERSION 0.27.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
include_directories(c-api/include)
link_directories(c-api/lib)

add_subdirectory(examples)

enable_testing()
add_executable(test-all tests/all.cc)
target_link_libraries(test-all wasmtime)
add_test(NAME smoke COMMAND test-all)
