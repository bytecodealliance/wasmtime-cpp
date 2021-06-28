cmake_minimum_required(VERSION 3.10)
project(Wasmtime VERSION 0.27.0)

option(CXX_STANDARD_20 "Use C++20" OFF)
message(STATUS "CXX_STANDARD_20       ${CXX_STANDARD_20}")

if (CXX_STANDARD_20)
set(CMAKE_CXX_STANDARD 20)
else()
set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_CODE_ANALYSIS "Run code analysis" OFF)
message(STATUS "ENABLE_CODE_ANALYSIS       ${ENABLE_CODE_ANALYSIS}")

function(configure_wasmtime_target name)
  if (MSVC)
    target_compile_options(${name} PUBLIC /DWASM_API_EXTERN= /DWASI_API_EXTERN=)
    target_link_libraries(${name} ws2_32 bcrypt advapi32 userenv ntdll shell32 ole32)
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
add_subdirectory(tests)
