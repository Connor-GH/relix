#include "pwd.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>

#define KiB (1024UL)
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)
#define TiB (1024 * GiB)

bool lflag = false;
bool iflag = false;
bool pflag = false;
bool hflag = false;
bool Fflag = false;
bool Lflag = false;

struct ls_time {
	uint64_t sec;
	uint64_t min;
	uint64_t hr;
	uint64_t day;
	uint64_t mo;
	uint64_t yr;
};

enum {
	FMT_FILE = 0,
	FMT_DIR = 2, // /
	FMT_LINK = 4, // @
	FMT_FIFO = 8, // |
	FMT_SOCK = 16, // =
	FMT_EXE = 32, // *
};

static char *
to_human_bytes(uint32_t number, char human_name[static 7])
{
	char letter;
	if (number > 1 * GiB) {
		letter = 'G';
		number /= 1 * GiB;
	} else if (number > 1 * MiB) {
		letter = 'M';
		number /= 1 * MiB;
	} else if (number > 1 * KiB) {
		letter = 'K';
		number /= 1 * KiB;
	} else {
		letter = 'B';
	}
	snprintf(human_name, 7, "%5u", number);
	human_name[5] = letter;
	human_name[6] = '\0';
	return human_name;
}

static char *
fmtname(char *path, int fmt_flag)
{
	static char buf[PATH_MAX + 1] = {};
	char *p;
	char *indicator = "";
	bool skip_fmt = false;
	// zero out the buffer.
	memset(buf, '\0', sizeof(buf));

	// Find first character after last slash.
	for (p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	if (strlen(p) >= PATH_MAX) {
		return p;
	}
	memmove(buf, p, strlen(p));
	switch (fmt_flag) {
	default:
	case FMT_FILE:
		skip_fmt = true;
		break;
	case FMT_DIR:
		indicator = "/";
		break;
	case FMT_LINK:
		indicator = "@";
		skip_fmt = true;
		char readlink_buf[PATH_MAX] = {};
		char sprintf_buf[PATH_MAX + sizeof(readlink_buf) + 4 + 2] = {};
		if (!Lflag) {
			if (readlink(buf, readlink_buf, PATH_MAX) < 0) {
				fprintf(stderr, "readlink `%s'", buf);
				perror("readlink");
				exit(1);
			}
			if (Fflag) {
				strncat(buf, indicator, 2);
			}
			strncpy(sprintf_buf, buf, sizeof(sprintf_buf));
			strncat(sprintf_buf, " -> ", 5);
			strncat(sprintf_buf, readlink_buf, sizeof(readlink_buf) + 1);
			snprintf(sprintf_buf, sizeof(sprintf_buf), "%s -> %s", buf, readlink_buf);
			memcpy(buf, sprintf_buf, PATH_MAX);
		}

		break;
	case FMT_SOCK:
		indicator = "=";
		if (!Fflag && pflag) {
			skip_fmt = true;
		}
		break;
	case FMT_FIFO:
		indicator = "|";
		if (!Fflag && pflag) {
			skip_fmt = true;
		}
		break;
	case FMT_EXE:
		indicator = "*";
		if (!Fflag && pflag) {
			skip_fmt = true;
		}
		break;
	}
	if (!skip_fmt) {
		strncat(buf, indicator, 2);
	}

	return buf;
}

static char *
mode_to_perm(mode_t mode, char ret[static 11])
{
	ret[0] = S_ISREG(mode)  ? '-' :
	         S_ISDIR(mode)  ? 'd' :
	         S_ISBLK(mode)  ? 'b' :
	         S_ISCHR(mode)  ? 'c' :
	         S_ISLNK(mode)  ? 'l' :
	         S_ISFIFO(mode) ? 'p' :
	                          '?';
	ret[1] = mode & S_IRUSR ? 'r' : '-';
	ret[2] = mode & S_IWUSR ? 'w' : '-';
	ret[3] = mode & S_IXUSR ? 'x' : '-';
	ret[4] = mode & S_IRGRP ? 'r' : '-';
	ret[5] = mode & S_IWGRP ? 'w' : '-';
	ret[6] = mode & S_IXGRP ? 'x' : '-';
	ret[7] = mode & S_IROTH ? 'r' : '-';
	ret[8] = mode & S_IWOTH ? 'w' : '-';
	ret[9] = mode & S_IXOTH ? 'x' : '-';
	ret[10] = '\0';
	return ret;
}
static void
ls_format(char *buf, struct stat st)
{
	int fmt_ret = 0;
	char human_bytes_buf[12];
	if (pflag || S_ISLNK(st.st_mode)) {
		switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			fmt_ret = FMT_DIR;
			break;
		case S_IFLNK:
			fmt_ret = FMT_LINK;
			break;
		case S_IFIFO:
			fmt_ret = FMT_FIFO;
			break;
		default:
			break;
		}
		if (fmt_ret == 0 && st.st_mode & 0111) {
			fmt_ret = FMT_EXE;
		}
	}
	if (lflag) {
		// inodes
		if (iflag) {
			fprintf(stdout, "%9d ", st.st_ino);
		}
		char ret[11];
		fprintf(stdout, "%s ", mode_to_perm(st.st_mode, ret));
		struct passwd *passwd = getpwuid(st.st_uid);
		if (passwd == NULL) {
			perror("getpwuid");
			exit(EXIT_FAILURE);
		}
		fprintf(stdout, "%u ", st.st_nlink);
		// TODO: add getgrgid (/etc/group) and use it here.
		fprintf(stdout, "%s %s ", passwd->pw_name, passwd->pw_name);
		if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode)) {
			if (hflag) {
				fprintf(stdout, "%s ", to_human_bytes(st.st_size, human_bytes_buf));
			} else {
				fprintf(stdout, "%6lu ", st.st_size);
			}
		} else {
			fprintf(stdout, " %2d,%2d ", major(st.st_dev), minor(st.st_dev));
		}
		struct tm *lt = localtime(&st.st_mtime);
		fprintf(stdout, "%02d-%02d %02d:%02d ", lt->tm_mon, lt->tm_mday,
		        lt->tm_hour, lt->tm_min);

		fprintf(stdout, "%s\n", fmtname(buf, fmt_ret));
	} else {
		fprintf(stdout, "%s ", fmtname(buf, fmt_ret));
	}
}

// this ls(1) tries to follow __minimal__ POSIX stuff.
static void
ls(char *path)
{
	int fd;
	struct dirent *de;
	struct stat st;
	char buf[PATH_MAX] = {};

	if ((fd = open(path, O_RDONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "ls: cannot open %s\n", path);
		perror("open");
		return;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "ls: cannot stat %s\n", path);
		perror("stat");
		close(fd);
		return;
	}

	switch (st.st_mode & S_IFMT) {
	case S_IFDIR: {
		if (strlen(path) + 1 > sizeof buf) {
			fprintf(stderr, "ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		char *p = buf + strlen(buf);
		*p++ = '/';
		DIR *dir = fdopendir(fd);
		if (dir == NULL) {
			perror("fdopendir");
			exit(EXIT_FAILURE);
		}
		while ((de = readdir(dir)) != NULL) {
			if (de->d_ino == 0) {
				continue;
			}
			strcpy(p, de->d_name);
			if ((Lflag ? stat : lstat)(buf, &st) < 0) {
				fprintf(stderr, "ls: cannot stat %s\n", buf);
				perror("stat");
				continue;
			}
			ls_format(buf, st);
		}
		closedir(dir);
	} break;
	default:
		ls_format(path, st);
		break;
	}
	fprintf(stdout, "%s", lflag ? "" : "\n");
	close(fd);
}

int
main(int argc, char *argv[])
{
	int i;
	// index for filenames or directory names
	int arg_idx = 1;

	if (argc < 2) {
		ls(".");
		return 0;
	}
	for (int j = 1; j < argc; j++) {
		if (argv[j][0] == '-' && argv[j][1] != '\0') {
			for (size_t k = 1; k < strlen(argv[j]); k++) {
				switch (argv[j][k]) {
				case 'l':
					lflag = true;
					break;
				case 'i':
					iflag = true;
					break;
				case 'F':
					Fflag = true;
					[[fallthrough]];
				case 'p':
					pflag = true;
					break;
				case 'h':
					hflag = true;
					break;
				case 'L':
					Lflag = true;
					break;
				default:
					break;
				}
			}
			arg_idx++;
		} else {
			break;
		}
	}
	if (arg_idx == argc) {
		ls(".");
		return 0;
	}

	for (i = arg_idx; i < argc; i++) {
		struct stat unused;
		if (stat(argv[i], &unused) >= 0) {
			ls(argv[i]);
		} else {
			int err_no = errno;
			printf("ls: can't open `%s'", argv[i]);
			errno = err_no;
			perror("");
		}
	}
	return 0;
}
