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
xsetenv(const char *name, const char *value, int overwrite)
{
	int ret = setenv(name, value, overwrite);
	if (ret != 0) {
		perror("setenv");
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char **argv)
{
	char username[LOGIN_NAME_MAX];
	char password[MAX_INPUT];
	struct passwd *entry;
	uid_t uid;
	bool uid_var_set = false;
	char *shell_path = "/bin/sh";
	char *homedir = "/";
	char *logname = "unknown";

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
	if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
	argc -= optind;
	argv -= optind;
	if (fflag) {
		entry = getpwnam(username);

		if (entry != NULL) {
			uid = entry->pw_uid;
			uid_var_set = true;
		}
		goto autologin;
	}
try_again:
	memset(username, 0, LOGIN_NAME_MAX);
	memset(password, 0, MAX_INPUT);
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

	if (fgets(password, MAX_INPUT, stdin) != password) {
		perror("fgets");
		exit(1);
	}
	password[strlen(password) - 1] = '\0';

	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag |= ECHO;
	tcsetattr(STDIN_FILENO, TCSADRAIN, &term);

	setpwent();

	if (!uid_var_set) {
		entry = getpwnam(username);

		if (entry != NULL) {
			uid = entry->pw_uid;
			uid_var_set = true;
		} else {
			fprintf(stderr, "User does not exist!\n");
			goto try_again;
		}
	}

	char *actual_password = "x";

	if (strcmp(password, actual_password) == 0) {
autologin:;
		if (getuid() == 0 && uid_var_set) {
			setuid(uid);
		}

		shell_path = entry != NULL && entry->pw_shell ? entry->pw_shell : "/bin/sh";
		homedir = entry != NULL && entry->pw_dir ? entry->pw_dir : "/";
		logname = entry != NULL && entry->pw_name ? entry->pw_name : "unknown";
		// cd $HOME || cd /
		chdir(homedir);
		xsetenv("HOME", homedir, 1);
		xsetenv("SHELL", shell_path, 1);
		xsetenv("LOGNAME", logname, 1);
		xsetenv("USER", logname, 1);
		char *const sh_argv[] = { shell_path, "-i", NULL };
		execve(shell_path, sh_argv, environ);
		printf("execv %s failed\n", shell_path);
		return 1;
	} else {
		fprintf(stderr, "Password is incorrect.\n");
		sleep(100);
		goto try_again;
	}
}
