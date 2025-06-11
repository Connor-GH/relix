#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
		cat(STDIN_FILENO);
		return 0;
	}

	for (int fd, i = 1; i < argc; i++) {
		if ((fd = open(argv[i], 0)) < 0) {
			int saved_errno = errno;
			fprintf(stderr, "%s: cannot open file: %s\n", argv[0],
			        strerror(saved_errno));
			return 0;
		}
		cat(fd);
		close(fd);
	}
	return 0;
}
