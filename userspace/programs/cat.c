#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

void
cat(int fd)
{
	char buf[512];
	int n;

	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		if (write(1, buf, n) != n) {
			fprintf(stdout, "cat: write error\n");
			exit(0);
		}
	}
	if (n < 0) {
		fprintf(stdout, "cat: read error\n");
		exit(0);
	}
}

int
main(int argc, char *argv[])
{
	int fd, i;

	if (argc <= 1) {
		cat(stdin);
		exit(0);
	}

	for (i = 1; i < argc; i++) {
		if ((fd = open(argv[i], 0)) < 0) {
			fprintf(stdout, "cat: cannot open %s\n", argv[i]);
			exit(0);
		}
		cat(fd);
		close(fd);
	}
	exit(0);
}
