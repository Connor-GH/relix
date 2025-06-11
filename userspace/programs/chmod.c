#include <stdio.h>
#include <stdlib.h> // for atoi_base (nonstandard)
#include <sys/stat.h>
// TODO atoi_base is pretty much strol

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "chmod [octal] file\n");
		return 0;
	}
	int mode = atoi_base(argv[1], 8);
	if (mode == 0) {
		fprintf(stderr, "Supply an octal node: e.g. 0677.\n");
		return 0;
	}
	int ret = chmod(argv[2], mode);
	(void)ret;
	return 0;
}
