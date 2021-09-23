//
// main.c
// By: Tristan Blaudez <tblaudez@student.codam.nl>
// Created: 15/09/2021 13:28:25
//

#include <memory.h> // memcpy
#include <stdio.h> // fputs
#include <unistd.h> // pid_t
#include <signal.h> // kill
#include "strace.h"

pid_t child = -1;

void start_child_process(int argc,  char **argv) {
	/* Create NULL-terminated array of args */
	char *args[argc + 1];
	memcpy(args,  argv,  argc * sizeof(char *));
	args[argc] = NULL;

	/* Wait for parent to seize child */
	kill(getpid(),  SIGSTOP);
	execvp(args[0],  args);

	/*We should never reach that point */
	strace_failure("execvp");
}

int main(int argc,  char **argv) {
	if (argc < 2) {
		fputs("usage: ./ft_strace <PROG> [ARGS]\n",  stderr);
		return 1;
	}

	signal(SIGINT,  signal_handler);

	switch ((child = fork())) {
		case -1:
			strace_failure("fork");
		case 0:
			start_child_process(--argc,  ++argv);
		default:
			return strace();
	}

}