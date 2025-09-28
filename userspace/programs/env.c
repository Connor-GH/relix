#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;

int
main(int argc, char **argv)
{
	if (argc == 1) {
		for (int i = 0; environ[i]; i++) {
			printf("%s\n", environ[i]);
		}
		exit(EXIT_SUCCESS);
	}
	int i = 1;
	for (; i < argc; i++) {
		if (strchr(argv[i], '=') != NULL) {
			char *name = strtok(argv[i], "=");
			char *value = strtok(NULL, "=");
			if (setenv(name, value, 1) < 0) {
				perror("env: setenv");
				exit(126);
			}
		} else {
			break;
		}
	}
	if (fork() == 0) {
		if (execvp(argv[i], argv + i) < 0) {
			perror("env: execvp");
			exit(127);
		}
	} else {
		wait(NULL);
	}

	return 0;
}
