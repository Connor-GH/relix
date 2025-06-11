// Demonstrate that moving the "acquire" in iderw after the loop that
// appends to the idequeue results in a race.

// For this to work, you should also add a spin within iderw's
// idequeue traversal loop.  Adding the following demonstrated a panic
// after about 5 runs of stressfs in QEMU on a 2.1GHz CPU:
//    for (i = 0; i < 40000; i++)
//      asm volatile("");

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(void)
{
	int fd, i;
	char path[] = "stressfs0";
	char data[512];

	fprintf(stdout, "stressfs starting\n");
	memset(data, 'a', sizeof(data));

	for (i = 0; i < 4; i++) {
		if (fork() > 0) {
			break;
		}
	}

	fprintf(stdout, "write %d\n", i);

	path[8] += i;
	fd = open(path, O_CREATE | O_RDWR);
	for (i = 0; i < 20; i++) {
		//    fprintf(fd, "%d\n", i);
		write(fd, data, sizeof(data));
	}
	close(fd);

	fprintf(stdout, "read\n");

	fd = open(path, O_RDONLY);
	for (i = 0; i < 20; i++) {
		read(fd, data, sizeof(data));
	}
	close(fd);

	wait(NULL);

	return 0;
}
