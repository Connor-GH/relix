// init: The initial user-level program

#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

char *argv[] = { "/bin/sh", 0 };

int
main(void)
{

	mkdir("/dev");
	if (open("/dev/console", O_RDWR) < 0) {
		mknod("/dev/console", 1, 1);
		open("/dev/console", O_RDWR);
	}
	// TODO: find a way of throwing an error if we cannot dup(0) ?
	(void)dup(0); // stdout
	(void)dup(0); // stderr
	fprintf(stdout, "/dev/console created\n");
	if (open("/dev/null", O_RDWR) < 0) {
		mknod("/dev/null", 2, 2);
	}
	fprintf(stdout, "/dev/null created\n");
	for (;;) {
		fprintf(stdout, "init: starting sh service\n");
		int pid = fork();
		if (pid < 0) {
			fprintf(stderr, "init: fork() failed\n");
			return 1;
		}
		if (pid == 0) {
			exec("/bin/sh", argv);
			fprintf(stderr, "init: exec() sh failed\n");
			return 1;
		}
		int wpid;
		while ((wpid = wait(NULL)) >= 0 && wpid != pid)
			fprintf(stderr, "zombie!\n");
	}
}
