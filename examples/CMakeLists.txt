function(add_example name)
  add_executable(${name} ${name}.cc)
  target_link_libraries(${name} wasmtime)
endfunction()

add_example(fuel)
add_example(hello)
add_example(interrupt)
add_example(linking)
