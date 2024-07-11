#include <stdio.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "%s: [file]\n", argv[0]);
		return 1;
	}
	int ret = unlink(argv[1]);
	if (ret == -1) {
		fprintf(stderr, "%s: failure to unlink %s\n", argv[0], argv[1]);
		return 1;
	}
	return 0;
}
