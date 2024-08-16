#include <stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

char buf[512];

static void
wc(int fd, const char *name)
{
	int i, n;
	int l, w, c, inword;

	l = w = c = 0;
	inword = 0;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		for (i = 0; i < n; i++) {
			c++;
			if (buf[i] == '\n')
				l++;
			if (strchr(" \r\t\n\v", buf[i]))
				inword = 0;
			else if (!inword) {
				w++;
				inword = 1;
			}
		}
	}
	if (n < 0) {
		fprintf(stderr, "wc: read error\n");
		exit(1);
	}
	fprintf(stdout, "%d %d %d %s\n", l, w, c, name);
}

int
main(int argc, const char *argv[])
{
	int fd, i;

	if (argc <= 1) {
		wc(0, "");
		return 0;
	}

	for (i = 1; i < argc; i++) {
		if ((fd = open(argv[i], 0)) < 0) {
			fprintf(stdout, "wc: cannot open %s\n", argv[i]);
			exit(1);
		}
		wc(fd, argv[i]);
		close(fd);
	}
	return 0;
}
