// init: The initial user-level program

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

extern char **environ;
char *const argv[] = { "/bin/getty", "-a", "root", "tty0", NULL };

static void
make_file_device_with_logging(const char *filename, dev_t dev_no, int flags,
															bool verbose)
{
	if (open(filename, flags) < 0) {
		mknod(filename, 0700 | S_IFCHR, dev_no);
	}
	if (verbose)
		fprintf(stdout, "%s created\n", filename);
}

static void
make_file_device_quiet(const char *filename, dev_t dev_no, int flags)
{
	make_file_device_with_logging(filename, dev_no, flags, false);
}
static void
make_file_device(const char *filename, dev_t dev_no, int flags)
{
	make_file_device_with_logging(filename, dev_no, flags, true);
}

void
execute_program(const char *program, char *const args[], char **env)
{
	pid_t pid = fork(); // Create a new process

	if (pid < 0) { // Check for fork error
		perror("fork failed");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) { // Child process
		execve(program, args, env); // Execute the program
		// If execvp is successful, this line will not be executed
		perror("execvp failed"); // Print error if execvp fails
		exit(EXIT_FAILURE);
	} else { // Parent process
		int status;
		wait(&status); // Wait for the child to finish

		if (WIFEXITED(status)) {
			printf("Child exited with status %d\n", WEXITSTATUS(status));
		} else {
			printf("Child did not terminate normally: %d\n", WEXITSTATUS(status));
		}
	}
}

int
main(void)
{
	mkdir("/dev", 0700);
	int fd;

	if ((fd = open("/dev/tty0", O_RDWR)) < 0) {
		mknod("/dev/tty0", 0700 | S_IFCHR, makedev(6, 0));
		fd = open("/dev/tty0", O_RDWR);
	}
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	// TODO: find a way of throwing an error if we cannot dup ?
	(void)dup(fd); // stdout
	(void)dup(fd); // stderr

	make_file_device_quiet("/dev/fb0", makedev(3, 0), O_RDWR);
	make_file_device_quiet("/dev/ttyS0", makedev(6, 64), O_RDWR);
	make_file_device("/dev/console", makedev(1, 1), O_RDWR);

	make_file_device("/dev/null", makedev(2, 1), O_RDWR);
	make_file_device("/dev/kbd0", makedev(4, 0), O_RDONLY | O_NONBLOCK);
	make_file_device("/dev/mouse0", makedev(7, 0), O_RDONLY | O_NONBLOCK);
	make_file_device("/dev/sda", makedev(5, 0), O_RDWR);

	for (;;) {
		fprintf(stdout, "init: starting getty service\n");
		int pid = fork();
		if (pid < 0) {
			fprintf(stderr, "init: fork() failed\n");
			return 1;
		}
		if (pid == 0) {
			execute_program("/bin/getty", argv, environ);
			return 1;
		}
		int wpid;
		while ((wpid = wait(NULL)) >= 0 && wpid != pid)
			fprintf(stderr, "zombie!\n");
	}
}
