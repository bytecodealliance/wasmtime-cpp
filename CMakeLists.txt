cmake_minimum_required(VERSION 3.14)
project(Wasmtime VERSION 0.27.0)

if (NOT CMAKE_CXX_STANDARD)
  message(STATUS "Cannot detect C++ Standard. Switching to C++17 by default !!")
  set(CMAKE_CXX_STANDARD 17)
endif()
message(STATUS "CMAKE_CXX_STANDARD is ${CMAKE_CXX_STANDARD}")
if (NOT CMAKE_CXX_STANDARD GREATER_EQUAL 17)
  message(FATAL_ERROR "WASMTIME_CPP library does not support ${CMAKE_CXX_STANDARD}")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_CODE_ANALYSIS "Run code analysis" OFF)
message(STATUS "ENABLE_CODE_ANALYSIS       ${ENABLE_CODE_ANALYSIS}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   add_compile_options (-fcolor-diagnostics)
endif ()

add_library(wasmtime-cpp INTERFACE ${CMAKE_SOURCE_DIR}/include/wasmtime.hh)
target_link_libraries(wasmtime-cpp INTERFACE wasmtime)
if (MSVC)
  target_compile_options(wasmtime-cpp INTERFACE /DWASM_API_EXTERN= /DWASI_API_EXTERN=)
  target_link_libraries(wasmtime-cpp INTERFACE ws2_32 bcrypt advapi32 userenv ntdll shell32 ole32)
else()
  target_link_libraries(wasmtime-cpp INTERFACE stdc++ pthread)
endif()

target_include_directories(
        wasmtime-cpp
        INTERFACE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/c-api/include)
target_link_directories(wasmtime-cpp INTERFACE ${PROJECT_SOURCE_DIR}/c-api/lib)

enable_testing()
add_subdirectory(examples)
add_subdirectory(tests)
