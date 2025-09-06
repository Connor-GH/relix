#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
usage(char **argv)
{
	fprintf(stderr, "usage: %s [-l|number]\n", argv[0]);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	if (argc <= 1) {
		usage(argv);
	}
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'l':
				for (int j = 0;; j++) {
					errno = 0;
					char *s = strerror(j);
					if (errno == 0) {
						printf("%s\n", s);
					} else {
						break;
					}
				}
				return 0;
			default:
				usage(argv);
				break;
			}
		}
	}
	int err = atoi(argv[1]);
	if (err == 0) {
		usage(argv);
	}

	errno = 0;
	char *s = strerror(err);
	if (errno == 0) {
		printf("%s\n", s);
	} else {
		usage(argv);
	}
	return 0;
}
