//
// print_syscall.c
// By: Tristan Blaudez <tblaudez@student.codam.nl>
// Created: 20/09/2021 13:04:35
//

#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <sys/reg.h>
#include <errno.h>
#include <linux/audit.h>
#include <stdbool.h>
#include "strace.h"

#define IS_WRITE_SYSCALL(x) (!strcmp(x->name, "write") || !strcmp(x->name, "pwrite64"))
#define IS_READ_SYSCALL(x) (!strcmp(x->name, "read") || !strcmp(x->name, "pread64"))

static const char *error_codes[] = {"", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO", "E2BIG", "ENOEXEC",
									"EBADF",
									"ECHILD", "EAGAIN", "ENOMEM", "EACCES", "EFAULT", "ENOTBLK", "EBUSY", "EEXIST",
									"EXDEV",
									"ENODEV", "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", "ENOTTY", "ETXTBSY",
									"EFBIG",
									"ENOSPC", "ESPIPE", "EROFS", "EMLINK", "EPIPE", "EDOM", "ERANGE", "EDEADLK",
									"ENAMETOOLONG", "ENOLCK", "ENOSYS", "ENOTEMPTY", "ELOOP", "ENOMSG", "EIDRM",
									"ECHRNG",
									"EL2NSYNC", "EL3HLT", "EL3RST", "ELNRNG", "EUNATCH", "ENOCSI", "EL2HLT", "EBADE",
									"EBADR",
									"EXFULL", "ENOANO", "EBADRQC", "EBADSLT", "EBFONT", "ENOSTR", "ENODATA", "ETIME",
									"ENOSR",
									"ENONET", "ENOPKG", "EREMOTE", "ENOLINK", "EADV", "ESRMNT", "ECOMM", "EPROTO",
									"EMULTIHOP",
									"EDOTDOT", "EBADMSG", "EOVERFLOW", "ENOTUNIQ", "EBADFD", "EREMCHG", "ELIBACC",
									"ELIBBAD",
									"ELIBSCN", "ELIBMAX", "ELIBEXEC", "EILSEQ", "ERESTART", "ESTRPIPE", "EUSERS",
									"ENOTSOCK",
									"EDESTADDRREQ", "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT", "EPROTONOSUPPORT",
									"ESOCKTNOSUPPORT", "EOPNOTSUPP", "EPFNOSUPPORT", "EAFNOSUPPORT", "EADDRINUSE",
									"EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH", "ENETRESET", "ECONNABORTED",
									"ECONNRESET",
									"ENOBUFS", "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS", "ETIMEDOUT",
									"ECONNREFUSED",
									"EHOSTDOWN", "EHOSTUNREACH", "EALREADY", "EINPROGRESS", "ESTALE", "EUCLEAN",
									"ENOTNAM",
									"ENAVAIL", "EISNAM", "EREMOTEIO", "EDQUOT", "ENOMEDIUM", "EMEDIUMTYPE", "ECANCELED",
									"ENOKEY", "EKEYEXPIRED", "EKEYREVOKED", "EKEYREJECTED", "EOWNERDEAD",
									"ENOTRECOVERABLE",
									"ERFKILL", "EHWPOISON"};
static uint64_t read_syscall_args[4];

static char *get_string(uint64_t argument, ssize_t size) {
	static char string[STRING_MAX_SIZE + 1];
	bzero(string, STRING_MAX_SIZE + 1);
	long data;

	if (size < 0 || size > STRING_MAX_SIZE)
		size = STRING_MAX_SIZE;

	for (int i = 0; i < size; i += sizeof(long)) {
		if (!(data = ptrace(PTRACE_PEEKDATA, child, argument + i, 0))) break;
		memcpy(string + i, &data, sizeof(long));
	}

	string[size] = '\0';
	return string;
}

void print_string(uint64_t argument, ssize_t size) {
	static const char *schar = "\a\b\t\n\v\f\r\\\0";
	static const char *echar[] = {"\\a", "\\b", "\\t", "\\n", "\\v", "\\f", "\\r", "\\\\", "\\0"};
	char *string = get_string(argument, size);
	char *tmp;

	if (size < 0)
		size = strlen(string);
	else if (size > STRING_MAX_SIZE)
		size = STRING_MAX_SIZE;

	fputc('\"', stderr);
	for (int i = 0; i < size; i++) {
		/* If the current character is a special character, escape it*/
		if ((tmp = strchr(schar, string[i])) != NULL)
			fputs(echar[tmp - schar], stderr);
			/* Else print it as is or as escaped octal */
		else if (isprint(string[i]))
			fputc(string[i], stderr);
		else
			fprintf(stderr, "\\%o", (uint8_t)string[i]);
	}
	fputc('\"', stderr);
	if (size == STRING_MAX_SIZE)
		fputs("...", stderr);
}

static void print_array(uint64_t argument) {
	uint64_t string_ptr;
	int array_size;

	for (array_size = 0;; array_size++) {
		if (!(ptrace(PTRACE_PEEKDATA, child, argument + (array_size * sizeof(long)), 0))) break;
	}
	if (array_size >= 5) {
		fprintf(stderr, "%p /* %d vars */", (void *)argument, array_size);
		return;
	}

	fputc('[', stderr);
	for (int i = 0;; i += sizeof(long)) {
		if (!(string_ptr = (uint64_t)ptrace(PTRACE_PEEKDATA, child, argument + i, 0))) break;
		print_string(string_ptr, -1);
		fputs(", ", stderr);
	}
	/* Remove trailing comma */
	fputs("\b\b]", stderr);
}

void print_argument(uint64_t argument, t_type type) {
	switch (type) {
		case INT:
			fprintf(stderr, "%d", (int)argument);
			break;
		case UINT:
			fprintf(stderr, "%u", (unsigned)argument);
			break;
		case LONG:
			fprintf(stderr, "%ld", (long)argument);
			break;
		case ULONG:
			fprintf(stderr, "%ld", (unsigned long)argument);
			break;
		case PTR:
			if (!argument)
				fputs("NULL", stderr);
			else
				fprintf(stderr, "%p", (void *)argument);
			break;
		case STR:
			print_string(argument, -1);
			break;
		case ARRAY:
			print_array(argument);
			break;
		case VOID:
		default:
			break;
	}
}

void print_syscall_return(struct ptrace_syscall_info *syscall_info) {
	long syscall_id = ptrace(PTRACE_PEEKUSER, child, ORIG_RAX * sizeof(long), 0);
	const t_syscall *syscall = find_syscall_by_id((uint64_t)syscall_id, syscall_info->arch);

	if (syscall && IS_READ_SYSCALL(syscall))
		print_read_syscall(syscall->name, read_syscall_args, syscall_info);

	if (syscall_info->exit.is_error) {
		if (-syscall_info->exit.rval < EHWPOISON)
			fprintf(stderr, " = -1 %s (%s)", error_codes[-syscall_info->exit.rval], strerror(
					(int)-syscall_info->exit.rval));
		else
			fprintf(stderr, " = -1 (%s)", strerror((int)-syscall_info->exit.rval));
	}
	else if (!syscall || syscall->return_type == VOID)
			asm("nop"); // Do nothing
	else {
		fputs(" = ", stderr);
		print_argument((uint64_t)syscall_info->exit.rval, syscall->return_type);
	}

	fputc('\n', stderr);
}

void print_syscall(struct ptrace_syscall_info *syscall_info) {
	const t_syscall *syscall = find_syscall_by_id(syscall_info->entry.nr, syscall_info->arch);
	static bool i386_process_message = false;

	if (syscall_info->arch == AUDIT_ARCH_I386 && i386_process_message == false) {
		fprintf(stderr, "ft_strace: [ Process PID=%d runs in 32 bit mode. ]\n", child);
		i386_process_message = true;
	}

	if (!syscall) {
		fprintf(stderr, "Unknown syscall (%llu)", syscall_info->entry.nr);
		return;
	}

	/* read() arguments are printed when the syscall returns */
	if (IS_READ_SYSCALL(syscall)) {
		memcpy(read_syscall_args, syscall_info->entry.args, sizeof(read_syscall_args));
		return;
	}

	fprintf(stderr, "%s(", syscall->name);

	if (syscall->arg_count != 0) {
		for (unsigned i = 0; i < syscall->arg_count; i++) {
			/* Special case for write syscall */
			if (IS_WRITE_SYSCALL(syscall) && i == 1) {
				print_string((long)syscall_info->entry.args[i], (ssize_t)syscall_info->entry.args[2]);
				fputs(", ", stderr);
				continue;
			}
			print_argument(syscall_info->entry.args[i], syscall->args_type[i]);
			fputs(", ", stderr);
		}
		/* Remove trailing comma */
		fputs("\b\b", stderr);
	}

	fputc(')', stderr);
}