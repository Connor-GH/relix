#include <unistd.h>
#include <stdio.h>

int
main(int argc, const char *argv[])
{
	int i;

	if (argc < 2) {
		fprintf(stderr, "Usage: rm files...\n");
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (unlink(argv[i]) < 0) {
			perror("rm failed");
			break;
		}
	}

	return 0;
}
