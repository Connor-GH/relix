// init: The initial user-level program

#include <user.h>
#include <fcntl.h>
#include <stdlib.h>

char *argv[] = { "/bin/sh", 0 };

int
main(void)
{
	int pid, wpid;

	if (open("console", O_RDWR) < 0) {
		mknod("console", 1, 1);
		open("console", O_RDWR);
	}
	dup(0); // stdout
	dup(0); // stderr

	for (;;) {
		fprintf(1, "init: starting sh service\n");
		pid = fork();
		if (pid < 0) {
			fprintf(1, "init: fork() failed\n");
			exit(0);
		}
		if (pid == 0) {
			exec("/bin/sh", argv);
			fprintf(1, "init: exec() sh failed\n");
			exit(0);
		}
		while ((wpid = wait(NULL)) >= 0 && wpid != pid)
			fprintf(1, "zombie!\n");
	}
}
