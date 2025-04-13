#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int
main(void)
{
	int fd;
	// Opens FIFO in blocking mode.
	// This will wait until there is a reader on the other end.
	fd = open("fifo1", O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	char c = 0;
	while (c <= 'Z') {
		if (read(fd, &c, 1) < 0) {
			break;
		} else {
			printf("received %c\n", c);
		}
	}
	close(fd);
	return 0;
}
