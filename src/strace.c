//
// strace.c
// By: Tristan Blaudez <tblaudez@student.codam.nl>
// Created: 20/09/2021 12:57:55
//

#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <signal.h>
#include <wait.h>
#include <stdio.h>
#include "strace.h"

static void init_sigsets(sigset_t *empty, sigset_t *blocked) {
	const int blocked_signals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTERM};

	sigemptyset(empty);
	sigemptyset(blocked);

	for (size_t i = 0; i < sizeof(blocked_signals) / sizeof(int); i++)
		sigaddset(blocked, blocked_signals[i]);
}

int strace(void) {
	struct ptrace_syscall_info syscall_info = {0};
	sigset_t empty, blocked;
	int status, exit_code;

	/* Initialize signal sets */
	init_sigsets(&empty, &blocked);

	/* Wait for child process to be ready */
	waitpid(child, &status, WUNTRACED);

	/* Seize child process. Mark them with 0x80 bitmask */
	ptrace(PTRACE_SEIZE, child, 0, PTRACE_O_TRACESYSGOOD);

	while (1) {
		/* Wait for next syscall */
		ptrace(PTRACE_SYSCALL, child, 0, 0);
		sigprocmask(SIG_SETMASK, &empty, 0);
		waitpid(child, &status, 0);
		sigprocmask(SIG_BLOCK, &blocked, 0);

		if (WIFEXITED(status) || WIFSIGNALED(status))
			break;
		else if (WIFSTOPPED(status) && !(WSTOPSIG(status) & 0x80)) {
			if (print_signal(&status)) break;
			else continue;
		}

		ptrace(PTRACE_GET_SYSCALL_INFO, child, sizeof(struct ptrace_syscall_info), &syscall_info);
		if (syscall_info.op == PTRACE_SYSCALL_INFO_ENTRY)
			print_syscall(&syscall_info);
		else if (syscall_info.op == PTRACE_SYSCALL_INFO_EXIT)
			print_syscall_return(&syscall_info);
	}

	fputs(" = ?\n", stderr);
	exit_code = get_exit_code(status);
	display_exit_code(status, exit_code);

	/* If child process was killed by signal then we kill ourselves with the same signal */
	if (WIFSIGNALED(status)) {
		kill(getpid(), WTERMSIG(status));
	}

	return exit_code;
}