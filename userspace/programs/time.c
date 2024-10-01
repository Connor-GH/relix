#include <ext.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/wait.h>

int
main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "usage: %s [program] [program_args]\n", argv[0]);
		exit(1);
	}
	int before = uptime();
	char *path = malloc(FILENAME_MAX);
	sprintf(path, "/bin/%s", argv[1]);
	if (fork() == 0)
		exec(path, argv+1);
	wait(NULL);
	printf("wall time %dms\n", (uptime() - before) );
	return 0;
}
