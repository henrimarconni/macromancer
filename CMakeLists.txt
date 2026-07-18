cmake_minimum_required(VERSION 3.15)
project(macromancer)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)

add_executable(
  macromancer
  src/main.c
  src/codegen.c
  src/parser.c
  src/vmem_arena.c
  src/diagnostics.c
  src/ce_getopt.c
)

target_include_directories(
  macromancer PRIVATE src
)

