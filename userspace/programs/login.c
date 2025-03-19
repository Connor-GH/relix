#include <sys/param.h>
#include <auth.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ext.h>
#include <unistd.h>
#include <stddef.h>

extern char **environ;

int
main(int argc, char **argv)
{
	char username[MAX_USERNAME];
	char passwd[MAX_PASSWD];
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
		uid = usertouid(username);
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
	echoout(0);
	if (fgets(passwd, MAX_PASSWD, stdin) != passwd) {
		perror("fgets");
		exit(1);
	}
	passwd[strlen(passwd) - 1] = '\0';
	echoout(1);
	if (uid == -1) {
		uid = usertouid(username);
		if (uid == -1) {
			fprintf(stderr, "User does not exist!\n");
			goto try_again;
		}
	}
	// usertopasswd allocates memory
	char *actual_password = userto_allocated_passwd(username);
	if (actual_password == NULL) {
		fprintf(stderr, "actual_password is NULL\n");
		exit(-1);
	}
	if (strcmp(passwd, actual_password) == 0) {
		fprintf(stdout, "Password is correct!\n");
		free(actual_password);
autologin:;
		setuid(uid);
		char *const sh_argv[] = { "/bin/sh", NULL };
		execve("/bin/sh", sh_argv, environ);
		printf("execv sh failed\n");
		return 1;
	} else {
		fprintf(stderr, "Password is incorrect.\n");
		sleep(100);
		free(actual_password);
		goto try_again;
	}
}
