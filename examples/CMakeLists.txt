function(add_example name)
  add_executable(${name} ${name}.cc)
  configure_wasmtime_target(${name})
  add_test(NAME example-${name} COMMAND ${name}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endfunction()

add_example(fuel)
add_example(interrupt)
add_example(linking)
