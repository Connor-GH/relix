#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(void)
{
	int pid = fork();
	int status;

	if (pid == 0) {
		// child
		printf("Exiting from child with 45\n");
		exit(45);
	} else if (pid > 0) {
		// parent
		printf("I am the parent, and the child is pid %d\n", pid);
		pid = wait(&status);
		if (pid == -1) {
			perror("wait");
			exit(-1);
		}
		printf("Child returned with %d: end of process: %d\n", WEXITSTATUS(status),
		       pid);
		return 0;
	} else {
		fprintf(stderr, "Error with fork()\n");
		exit(-1);
	}
}
