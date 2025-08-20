#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	char *buffer = malloc(SYMLINK_MAX);
	if (buffer == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	if (argc == 1) {
		fprintf(stderr, "usage: %s FILE\n", argv[0]);
		free(buffer);
		return 1;
	}
	if (readlink(argv[1], buffer, SYMLINK_MAX) < 0) {
		perror("readlink");
		free(buffer);
		return 2;
	}

	buffer[SYMLINK_MAX - 1] = '\0';
	printf("%s\n", buffer);
	free(buffer);
	return 0;
}
