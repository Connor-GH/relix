#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int
main(void)
{
	// Open for writing.
	// This will either succeed, or return with broken pipe.
	int fd = open("fifo1", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	char data = 'A';
	while (data <= 'Z') {
		if (write(fd, &data, sizeof(data)) < 0) {
			break;
		} else {
			fprintf(stdout, "Wrote %c from pid %d\n", data, getpid());
		}
		data++;
	}
	close(fd);
	return 0;
}
