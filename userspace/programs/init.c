// init: The initial user-level program

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

extern char *const *environ;
char *const argv[] = { "/bin/sh", NULL };

int
main(void)
{
	mkdir("/dev", 0700);
	int fd;
	if ((fd = open("/dev/console", O_RDWR)) < 0) {
		mknod("/dev/console", 0700, makedev(1, 1));
		fd = open("/dev/console", O_RDWR);
	}
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	// TODO: find a way of throwing an error if we cannot dup ?
	(void)dup(fd); // stdout
	(void)dup(fd); // stderr
	fprintf(stdout, "/dev/console created\n");
	if (open("/dev/null", O_RDWR) < 0) {
		mknod("/dev/null", 0700, makedev(2, 1));
	}
	fprintf(stdout, "/dev/null created\n");
	if (open("/dev/fb0", O_RDWR) < 0) {
		mknod("/dev/fb0", 0700, makedev(3, 0));
	}
	fprintf(stdout, "/dev/fb0 created\n");
	for (;;) {
		fprintf(stdout, "init: starting sh service\n");
		int pid = fork();
		if (pid < 0) {
			fprintf(stderr, "init: fork() failed\n");
			return 1;
		}
		if (pid == 0) {
			execve("/bin/sh", argv, environ);
			perror("execve");
			fprintf(stderr, "init: exec() sh failed\n");
			return 1;
		}
		int wpid;
		while ((wpid = wait(NULL)) >= 0 && wpid != pid)
			fprintf(stderr, "zombie!\n");
	}
}
