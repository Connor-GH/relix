#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

extern char **environ;
int
main(int argc, char **argv)
{
	char c;
	bool aflag = false;
	char *user = NULL;
	while ((c = getopt(argc, argv, "a:")) != -1) {
		switch (c) {
		case 'a':
			aflag = true;
			user = optarg;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;
	char *termname = argv[0];
	// Set $TERM
	if (termname != NULL) {
		setenv("TERM", termname, 1);
	}

	if (aflag) {
		execve("/bin/login", (char *const []){"/bin/login", "-f", user, NULL}, environ);
	} else {
		execve("/bin/login", (char *const []){"/bin/login", NULL}, environ);
	}
	perror("execve");
	fprintf(stderr, "init: exec() login failed\n");

	return 0;

}
