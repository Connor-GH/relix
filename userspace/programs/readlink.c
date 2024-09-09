#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	char *buffer = malloc(DIRSIZ);
	if (argc == 1) {
		fprintf(stderr, "usage: %s FILE\n", argv[0]);
		return 1;
	}
	if (readlink(argv[1], buffer, DIRSIZ) < 0) {
		perror("readlink");
		free(buffer);
		return 2;
	}

	buffer[DIRSIZ - 1] = '\0';
	printf("%s\n", buffer);
	free(buffer);
	return 0;
}
