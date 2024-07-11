#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
usage(char **argv)
{
	fprintf(stderr, "usage: %s [-l|number]\n", argv[0]);
	exit(-1);
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
				for (int j = 0; j < sizeof(errno_codes) / sizeof(errno_codes[0]); j++) {
					printf("%s\n", errno_codes[j]);
				}
				return 0;
			default:
				usage(argv);
				break;
			}
		}
	}
	int err = atoi(argv[1]);
	if (err == 0)
		usage(argv);
	if (err > MAX_ERRNO - 1)
		usage(argv);
	printf("%s\n", errno_codes[err]);

	return 0;
}
