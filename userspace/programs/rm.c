#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *progname;

static void
set_progname(char *progname_)
{
	progname = progname_;
}

static bool iflag = false;
static bool force = false;
static bool Rflag = false;
static bool rflag = false;

// A directory is empty if it only has "." and "..".
static bool
dir_exists_and_is_empty(const char *path)
{
	struct dirent *dt;
	DIR *dp = opendir(path);
	if (dp == NULL) {
		return false;
	}
	dt = readdir(dp);
	for (; dt != NULL; dt = readdir(dp)) {
		if (strncmp(dt->d_name, ".", 1) != 0 && strncmp(dt->d_name, "..", 2) != 0) {
			closedir(dp);
			return true;
		}
	}
	closedir(dp);
	return false;
}

static void
rm_maybe_error(const char *path, int ret)
{
	if (ret < 0) {
		fprintf(stderr, "%s: failed to remove '%s': %s\n", progname, path,
		        strerror(errno));
	}
}

static void
prompt_delete(const char *path, bool isdir)
{
	char buf[32];
	fprintf(stderr, "%s: remove '%s'? ", progname, path);
	fgets(buf, sizeof(buf), stdin);
	if (strncmp(buf, "y", 1) == 0) {
		int ret = (isdir ? rmdir : unlink)(path);
		rm_maybe_error(path, ret);
	}
}

static void
delete_file(const char *path, bool isdir, bool willprompt)
{
	if (willprompt) {
		prompt_delete(path, isdir);
	} else {
		int ret = (isdir ? rmdir : unlink)(path);
		rm_maybe_error(path, ret);
	}
}

static void
rm(const char *path)
{
	struct stat st;
	int ret = stat(path, &st);

	if (ret < 0) {
		// "-f" is silent on errors.
		if (!force) {
			rm_maybe_error(path, ret);
			// Go on to any remaining files.
			return;
		}
	}

	if (S_ISDIR(st.st_mode)) {
		if (!Rflag && !rflag) {
			rm_maybe_error(path, ret);
			return;
		}
		if (dir_exists_and_is_empty(path)) {
			goto step_2d;
		}

		struct dirent *dt;
		DIR *dp = opendir(path);
		if (dp == NULL) {
			return;
		}
		dt = readdir(dp);
		for (; dt != NULL; dt = readdir(dp)) {
			if (strncmp(dt->d_name, ".", 1) != 0 &&
			    strncmp(dt->d_name, "..", 2) != 0) {
				rm(dt->d_name);
			}
		}
		closedir(dp);

step_2d:
		delete_file(path, true, iflag);
		return;
	} else {
		delete_file(path, false,
		            !force && (((st.st_mode & 0222) == 0 && isatty(STDIN_FILENO)) ||
		                       iflag));
	}
}

int
main(int argc, char *argv[])
{
	set_progname(argv[0] ? argv[0] : "rm");

	if (argc < 2) {
		fprintf(stderr, "Usage: rm files...\n");
		exit(EXIT_FAILURE);
	}
	int c;
	while ((c = getopt(argc, argv, "fiRr")) != -1) {
		switch (c) {
		case 'i':
			iflag = true;
			break;
		case 'f':
			force = true;
			break;
		case 'R':
			Rflag = true;
			break;
		case 'r':
			rflag = true;
			break;
		default:
			break;
		}
	}

	argc -= optind;
	argv += optind;

	for (int i = 0; i < argc; i++) {
		rm(argv[i]);
	}
	return 0;
}
