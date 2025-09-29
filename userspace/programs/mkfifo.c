#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	char *argv0 = argv[0];
	char c;
	mode_t mode = 0666; // rw user, group, and others
	while ((c = getopt(argc, argv, "m:")) != -1) {
		switch (c) {
		case 'm':
			errno = 0;
			mode = strtol(optarg, NULL, 8);
			if (errno != 0) {
				mode = 0666;
			}
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		fprintf(stderr, "Usage: %s [-m mode]\n", argv0 ? argv0 : "mkfifo");
		return 1;
	}
	if (mkfifo(argv[0], mode) < 0) {
		perror("mkfifo");
		exit(EXIT_FAILURE);
	}
	return 0;
}
