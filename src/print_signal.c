//
// print_signal.c
// By: Tristan Blaudez <tblaudez@student.codam.nl>
// Created: 22/09/2021 16:06:58
//

#include <signal.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <wait.h>
#include "strace.h"

const char *signal_table[] = {
		"", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1",
		"SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP",
		"SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR",
		"SIGSYS"
};

int print_signal(int *status) {
	siginfo_t siginfo;

	/* Get information about the signal that stopped the child process */
	if (ptrace(PTRACE_GETSIGINFO, child, 0, &siginfo) == -1)
		strace_failure("PTRACE_GETSIGINFO");

	/* If signal is not one of our own SIGTRAP, display it */
	if (!(siginfo.si_signo == SIGTRAP && siginfo.si_pid == child)) {
		fprintf(stderr,
				"--- %s {si_signo=%d, si_code=%d, si_pid=%d, si_uid=%d, si_status=%d, si_utime=%ld, si_stime=%ld} ---\n",
				signal_table[siginfo.si_signo], siginfo.si_signo, siginfo.si_code, siginfo.si_pid, siginfo.si_uid,
				siginfo.si_status, siginfo.si_utime, siginfo.si_stime);
	}

	/* Resume child process with the trapped signal */
	if (siginfo.si_signo == SIGCONT)
		ptrace(PTRACE_CONT, child, 0, siginfo.si_signo);

	/* If signal is SIGSTOP, put the child process in "stopped" state */
	if (siginfo.si_signo == SIGSTOP) {
		waitpid(child, status, WUNTRACED);
		ptrace(PTRACE_LISTEN, child, 0, 0);
		waitpid(child, status, WCONTINUED);
	}

	return WIFEXITED(*status) || WIFSIGNALED(*status);
}