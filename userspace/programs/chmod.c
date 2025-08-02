#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "chmod [octal] file\n");
		exit(EXIT_FAILURE);
	}
	errno = 0;
	int mode = (int)strtol(argv[1], NULL, 8);
	if (errno != 0) {
		perror("strtol");
		fprintf(stderr, "Supply an octal node: e.g. 0677.\n");
		exit(EXIT_FAILURE);
	}
	int ret = chmod(argv[2], mode);
	if (ret < 0) {
		perror("chmod");
		exit(EXIT_FAILURE);
	}
	return 0;
}
