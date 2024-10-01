#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <kernel/include/fs.h>

int
main(void)
{
	char buf[BSIZE];
	int fd, i, sectors;

	fd = open("big.file", O_CREATE | O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "big: cannot open big.file for writing\n");
		exit(1);
	}

	sectors = 0;
	while (sectors <= (NDIRECT + NINDIRECT)) {
		*(int *)buf = sectors;
		int cc = write(fd, buf, sizeof(buf));
		if (cc <= 0) {
			perror("write");
			break;
		}
		sectors++;
		if (sectors % 100 == 0)
			fprintf(stderr, ".");
	}

	fprintf(stdout, "\nwrote %d blocks\n", sectors);

	close(fd);
	fd = open("big.file", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "big: cannot re-open big.file for reading\n");
		exit(1);
	}
	for (i = 0; i < sectors; i++) {
		int cc = read(fd, buf, sizeof(buf));
		if (cc <= 0) {
			fprintf(stderr, "big: read error at sector %d\n", i);
			exit(1);
		}
		if (*(int *)buf != i) {
			fprintf(stderr, "big: read the wrong data (%d) for sector %d\n",
							*(int *)buf, i);
			exit(1);
		}
	}

	exit(0);
}
