#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

// remove "SYS_"
// sizeof minus one because of the null terminator
#define REMOVE_SYS_IN_NAME(x) ((char *)(#x) + (sizeof("SYS_") - 1))

// read -> 5 for example
int
from_name_to_digit(char *string)
{
	for (int i = 0; i < SYSCALL_AMT; i++) {
		if (strcmp(syscall_names[i], string) == 0)
			return i;
	}
	return -1;
}

int
main(int argc, char **argv)
{
	char ptrace_flags[SYSCALL_AMT] = { 0 };

	int argv_count_start = 0;
	if (argc < 4) {
		fprintf(stderr,
						"Not enough arguments supplied.\n"
						"Usage: ptrace [syscall] [/path/to/prog] [prog] [arguments]\n");
		return 1;
	}
	for (int i = 1; i < argc; i++) {
		if (from_name_to_digit(argv[i]) != -1) {
			ptrace_flags[from_name_to_digit(argv[i])] = 1;
		} else {
			argv_count_start = i;
			break;
		}
	}

	if (ptrace(ptrace_flags) < 0) {
		fprintf(stderr, "ptrace(2) failed!\n");
		return 1;
	}
	execv(argv[argv_count_start], argv + argv_count_start + 1);
	return 0;
}
