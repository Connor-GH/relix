#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "%s: [file]\n", argv[0]);
		exit(-1);
	}

	int ret = sleep(atoi(argv[1]) * 1000);
	while (ret != 0) {
		ret = sleep(ret);
	}
	return 0;
}
