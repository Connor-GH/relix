#include "termios.h"
#include <ext.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

extern char **environ;

int
main(int argc, char **argv)
{
	char username[MAX_USERNAME];
	char passwd[MAX_PASSWD];
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
	memset(username, 0, MAX_USERNAME);
	memset(passwd, 0, MAX_PASSWD);
	printf("username: ");
	if (fgets(username, MAX_USERNAME, stdin) != username) {
		perror("fgets");
		exit(1);
	}
	username[strlen(username) - 1] = '\0';

	printf("password for %s: ", username);
	struct termios term;

	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSADRAIN, &term);

	if (fgets(passwd, MAX_PASSWD, stdin) != passwd) {
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
		fprintf(stdout, "Password is correct!\n");
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
