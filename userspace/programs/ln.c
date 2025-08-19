#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	char c;
	bool sflag = false;
	while ((c = getopt(argc, argv, "s")) != -1) {
		switch (c) {
		case 's':
			fprintf(stderr, "WARNING: ln -s is experimental!\n");
			sflag = true;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2) {
		fprintf(stderr, "Usage: ln [-s] old new\n");
		return 1;
	}
	if (sflag) {
		if (symlink(argv[0], argv[1]) < 0) {
			fprintf(stderr, "ln -s %s %s: failed\n", argv[0], argv[1]);
			perror("symlink");
		}

	} else {
		if (link(argv[0], argv[1]) < 0) {
			fprintf(stderr, "ln %s %s: failed\n", argv[0], argv[1]);
			perror("link");
		}
	}
	return 0;
}
