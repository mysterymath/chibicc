cmake_minimum_required(VERSION 3.18)
set(LLVM_MOS_PLATFORM cx16)
find_package(llvm-mos-sdk REQUIRED)
project(chibicc)

add_executable(chibicc
  codegen.c
  main.c
  parse.c
  std.c
  type.c
  tokenize.c
)

add_executable(as
  as.c
  as-tokenize.c
  std.c
)
target_include_directories(as SYSTEM PRIVATE sysroot/include)
