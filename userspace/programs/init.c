// init: The initial user-level program

#include "../include/user.h"
#include "../../include/fcntl.h"

char *argv[] = { "login", 0 };

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
		fprintf(1, "init: starting login service\n");
		pid = fork();
		if (pid < 0) {
			fprintf(1, "init: fork() failed\n");
			exit();
		}
		if (pid == 0) {
			exec("login", argv);
			fprintf(1, "init: exec() login service failed\n");
			exit();
		}
		while ((wpid = wait()) >= 0 && wpid != pid)
			fprintf(1, "zombie!\n");
	}
}
