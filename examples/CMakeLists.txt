function(add_example name)
  add_executable(${name} ${name}.cc)
  configure_wasmtime_target(${name})
endfunction()

add_example(fuel)
add_example(hello)
add_example(interrupt)
add_example(linking)
