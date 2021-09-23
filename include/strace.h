//
// strace.h
// By: Tristan Blaudez <tblaudez@student.codam.nl>
// Created: 15/09/2021 13:29:57
//

#pragma once

#include <unistd.h> // pid_t
#include <stdint.h> // uint64_t
#include <linux/ptrace.h> //ptrace_syscall_info
#include <sys/types.h> // pid_t

#define STRING_MAX_SIZE 32
#define SYSCALL_MAX_ARGS 6

typedef enum {
	INT, UINT, LONG, ULONG, PTR, STR, ARRAY, VOID
} t_type;

typedef struct {
	const char *name;
	t_type return_type;
	unsigned arg_count;
	t_type args_type[SYSCALL_MAX_ARGS];
} t_syscall;

extern pid_t child;

// main.c
void start_child_process(int argc, char **argv) __attribute__((noreturn));

// syscall_table.c
const t_syscall *find_syscall_by_id(uint64_t syscall_id, uint32_t arch);

// print_syscall.c
void print_syscall(struct ptrace_syscall_info *syscall_info);
void print_syscall_return(struct ptrace_syscall_info *syscall_info);
void print_string(uint64_t argument, ssize_t size);

// print_signal.c
int print_signal(int *status);

// strace.c
int strace(void);

// utils.c
void strace_failure(const char *error_str) __attribute__((noreturn));
void signal_handler(int signal);
void print_read_syscall(const char *name, uint64_t args[4], struct ptrace_syscall_info *syscall_info);
int get_exit_code(int status);
void display_exit_code(int status, int exit_code);