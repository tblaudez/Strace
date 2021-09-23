cmake_minimum_required(VERSION 3.16)
project(Strace C)

set(CMAKE_C_STANDARD 99)

set(INCLUDE_FILES include/strace.h)
set(SOURCE_FILES src/main.c src/utils.c src/strace.c src/print_syscall.c src/syscall_table.c src/print_signal.c)

add_executable(ft_strace ${SOURCE_FILES} ${INCLUDE_FILES})
target_include_directories(ft_strace PRIVATE include/)