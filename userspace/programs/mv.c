#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: %s [old] [new]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int ret = rename(argv[1], argv[2]);
	if (ret < 0) {
		perror("rename");
		exit(EXIT_FAILURE);
	}
	return 0;
}
