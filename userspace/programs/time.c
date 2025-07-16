#include <ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "usage: %s [program] [program_args]\n", argv[0]);
		exit(1);
	}
	time_t before = uptime();
	char *path = malloc(FILENAME_MAX);
	if (path == NULL) {
		perror("malloc");
		exit(1);
	}
	sprintf(path, "/bin/%s", argv[1]);
	if (fork() == 0) {
		execv(path, argv + 1);
	}
	wait(NULL);
	printf("wall time %ldms\n", (uptime() - before));
	return 0;
}
