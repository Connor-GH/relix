#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

extern char *optarg;
extern int optind, opterr, optopt;

static bool
test_zero(char *s)
{
	return s == NULL || s[0] == '\0';
}

static bool
test_not_zero(char *s)
{
	return !test_zero(s);
}

static __attribute__((noreturn)) void
usage(void)
{
	fprintf(stderr, "usage: [ (-z string | -n string) ]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	bool zflag, nflag;
	zflag = nflag = false;
	int c;
	char *my_string;
	while ((c = getopt(argc, argv, "z:n:")) != -1) {
		switch (c) {
		case 'z':
			zflag = 1;
			my_string = optarg;
			break;
		case 'n':
			nflag = 1;
			my_string = optarg;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	if (zflag && nflag)
		usage();

	if (strcmp(argv[0], "]") == 0) {
		if (zflag)
			return test_zero(my_string);
		if (nflag)
			return test_not_zero(my_string);
	}
	usage();
}
