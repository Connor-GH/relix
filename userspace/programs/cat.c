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
			perror("cat");
			exit(1);
		}
	}
	if (n < 0) {
		perror("cat");
		exit(1);
	}
}

int
main(int argc, const char *argv[])
{
	if (argc <= 1) {
		cat(stdin);
		return 0;
	}

	for (int fd, i = 1; i < argc; i++) {
		if ((fd = open(argv[i], 0)) < 0) {
			perror("cat: cannot open file");
			return 0;
		}
		cat(fd);
		close(fd);
	}
	return 0;
}
