module test;
import bindings.stdio;
import bindings.stdlib;

extern(C) {
	int getopt(int argc, char **argv, const char *optstring);
	__gshared char *optarg;
	__gshared int optind, opterr, optopt;
	noreturn exit(int status);
	size_t strcmp(const char *s, const char *c);
}

static bool test_zero(char *s) => s is null || s[0] == '\0';

static bool test_not_zero(char *s) => !test_zero(s);

static noreturn
usage()
{
	fprintf(stderr, "usage: [ (-z string | -n string) ]\n");
	exit(1);
}

extern(C) int main(int argc, char **argv) {
	optind = 1;
	opterr = 1;
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

	// flags are mutally exclusive
	if (zflag * nflag)
		usage();

	if (strcmp(argv[0], "]") == 0) {
		if (zflag)
			return test_zero(my_string);
		if (nflag)
			return test_not_zero(my_string);
	}
	usage();
}
