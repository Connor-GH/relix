#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	argv++;

	if (argc != 2) {
		fprintf(stderr, "Usage: link file1 file2\n");
		exit(EXIT_FAILURE);
	}
	if (link(argv[0], argv[1]) < 0) {
		fprintf(stderr, "ln -s %s %s: failed\n", argv[0], argv[1]);
		perror("symlink");
		exit(EXIT_FAILURE);
	}

	return 0;
}
