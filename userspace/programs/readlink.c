#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

[[noreturn]] static void
usage(char *progname)
{
	fprintf(stderr, "usage: %s [-n] file\n", progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	char *progname = argv[0] ? argv[0] : "readlink";
	bool newline = true;
	char buffer[SYMLINK_MAX];
	if (argc == 1) {
		usage(progname);
	}
	int c;
	while ((c = getopt(argc, argv, "n")) != -1) {
		switch (c) {
		case 'n':
			newline = false;
			break;
		default:
			usage(progname);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (readlink(argv[1], buffer, SYMLINK_MAX) < 0) {
		perror("readlink");
		return 2;
	}

	buffer[SYMLINK_MAX - 1] = '\0';
	printf("%s%s", buffer, newline ? "\n" : "");
	return 0;
}
