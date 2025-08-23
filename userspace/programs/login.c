#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

extern char **environ;

static void
noop(int signum)
{
}

int
main(int argc, char **argv)
{
	char username[LOGIN_NAME_MAX];
	char passwd[MAX_INPUT];
	struct passwd *entry;
	int uid = -1;

	char c;
	bool fflag = false;
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			fflag = true;
			memcpy(username, optarg, sizeof(username));
			break;
		default:
			break;
		}
	}
	if (signal(SIGINT, noop) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
	argc -= optind;
	argv -= optind;
	if (fflag) {
		entry = getpwnam(username);

		if (entry != NULL) {
			uid = entry->pw_uid;
		}
		goto autologin;
	}
try_again:
	memset(username, 0, LOGIN_NAME_MAX);
	memset(passwd, 0, MAX_INPUT);
	printf("username: ");
	if (fgets(username, LOGIN_NAME_MAX, stdin) != username) {
		perror("fgets");
		exit(1);
	}
	username[strlen(username) - 1] = '\0';

	printf("password for %s: ", username);
	struct termios term;

	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSADRAIN, &term);

	if (fgets(passwd, MAX_INPUT, stdin) != passwd) {
		perror("fgets");
		exit(1);
	}
	passwd[strlen(passwd) - 1] = '\0';

	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag |= ECHO;
	tcsetattr(STDIN_FILENO, TCSADRAIN, &term);

	if (uid == -1) {
		entry = getpwnam(username);

		if (entry != NULL) {
			uid = entry->pw_uid;
		}
		if (uid == -1) {
			fprintf(stderr, "User does not exist!\n");
			goto try_again;
		}
	}

	char *actual_password = "x";

	if (strcmp(passwd, actual_password) == 0) {
autologin:;
		if (uid == 0) {
			setuid(uid);
		}
		char *const sh_argv[] = { "/bin/sh", NULL };
		execve("/bin/sh", sh_argv, environ);
		printf("execv sh failed\n");
		return 1;
	} else {
		fprintf(stderr, "Password is incorrect.\n");
		sleep(100);
		goto try_again;
	}
}
