//
// utils.c
// By: Tristan Blaudez <tblaudez@student.codam.nl>
// Created: 20/09/2021 12:48:23
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <limits.h>
#include "strace.h"

extern const char *signal_table[];

void strace_failure(const char *error_str) {
	fprintf(stderr, "ft_strace: %s: %s\n", error_str, strerror(errno));
	exit(EXIT_FAILURE);
}

void signal_handler(int signal) {
	if (signal == SIGINT) {
		fprintf(stderr, "ft_strace: process %d detached\n\n", child);
		kill(child, SIGINT);
		exit(EXIT_SUCCESS);
	}
}

/* I know this code is awful but I am way past the point of giving a fuck */
void print_read_syscall(const char *name, uint64_t args[4], struct ptrace_syscall_info *syscall_info) {
	fprintf(stderr, "%s(%lu, ", name, args[0]);
	if (!syscall_info->exit.is_error)
		print_string(args[1], syscall_info->exit.rval);
	else
		fprintf(stderr, "%p", (void *)args[1]);
	fprintf(stderr, ", %lu", args[2]);
	if (!strcmp(name, "pread64"))
		fprintf(stderr, ", %lu", args[3]);
	fputc(')', stderr);
}

int get_exit_code(int status) {
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		return CHAR_MAX + 1 + WTERMSIG(status);
	/* We should never reach this point */
	return EXIT_FAILURE;
}

void display_exit_code(int status, int exit_code) {
	if (WIFSIGNALED(status)) {
		fprintf(stderr, "+++ Killed by %s ", signal_table[WTERMSIG(status)]);
		if (WCOREDUMP(status))
			fputs(" (core dumped) ", stderr);
		fputs("+++\n", stderr);
	} else if (WIFEXITED(status))
		fprintf(stderr, "+++ exited with %d +++\n", exit_code);
}