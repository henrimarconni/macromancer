cmake_minimum_required(VERSION 3.15)
project(macromancer)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
  macromancer
  src/main.c
  src/codegen.c
  src/parser.c
  src/vmem_arena.c
)

target_include_directories(
  macromancer PRIVATE src
)

