#include <sys/stat.h>
#include <kernel/include/fs.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define KiB (1024UL)
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)
#define TiB (1024 * GiB)
struct ls_time {
	uint sec;
	uint min;
	uint hr;
	uint day;
	uint mo;
	uint yr;
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
to_human_bytes(uint number, char human_name[static 7])
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
	sprintf(human_name, "% 5u", number);
	human_name[5] = letter;
	human_name[6] = '\0';
	return human_name;
}

static struct ls_time
unix_time_to_human_readable(uint unix_seconds)
{
	// Save the time in Human
	// readable format
	struct ls_time lt;

	// Number of days in month
	// in normal year
	const int days_of_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int curr_year, days_till_now, extra_time, extra_days, index, date, month,
		hours, minutes, seconds, flag = 0;

	// Calculate total days unix time T
	days_till_now = unix_seconds / (24 * 60 * 60);
	extra_time = unix_seconds % (24 * 60 * 60);
	curr_year = 1970;

	// Calculating current year
	while (true) {
		if (curr_year % 400 == 0 || (curr_year % 4 == 0 && curr_year % 100 != 0)) {
			if (days_till_now < 366) {
				break;
			}
			days_till_now -= 366;
		} else {
			if (days_till_now < 365) {
				break;
			}
			days_till_now -= 365;
		}
		curr_year += 1;
	}

	// Updating extradays because it
	// will give days till previous day
	// and we have include current day
	extra_days = days_till_now + 1;

	if (curr_year % 400 == 0 || (curr_year % 4 == 0 && curr_year % 100 != 0))
		flag = 1;

	// Calculating MONTH and DATE
	month = 0;
	index = 0;
	if (flag == 1) {
		while (true) {
			if (index == 1) {
				if (extra_days - 29 < 0)
					break;

				month += 1;
				extra_days -= 29;
			} else {
				if (extra_days - days_of_month[index] < 0) {
					break;
				}
				month += 1;
				extra_days -= days_of_month[index];
			}
			index += 1;
		}
	} else {
		while (true) {
			if (extra_days - days_of_month[index] < 0) {
				break;
			}
			month += 1;
			extra_days -= days_of_month[index];
			index += 1;
		}
	}

	// Current Month
	if (extra_days > 0) {
		month += 1;
		date = extra_days;
	} else {
		if (month == 2 && flag == 1)
			date = 29;
		else {
			date = days_of_month[month - (month >= 1 ? 1 : 0)];
		}
	}

	hours = extra_time / 3600;
	minutes = (extra_time % 3600) / 60;
	seconds = (extra_time % 3600) % 60;

	lt.day = date;
	lt.mo = month;
	lt.yr = curr_year;
	lt.hr = hours;
	lt.min = minutes;
	lt.sec = seconds;

	// Return the time
	return lt;
}

static char *
fmtname(char *path, int fmt_flag)
{
	static char buf[DIRSIZ + 1];
	char *p;
	char indicator;
	bool skip_fmt = false;
	// zero out the buffer.
	memset(buf, '\0', sizeof(buf));

	// Find first character after last slash.
	for (p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	if (strlen(p) >= DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	switch (fmt_flag) {
	default:
	case FMT_FILE:
		skip_fmt = true;
		break;
	case FMT_DIR:
		indicator = '/';
		break;
	case FMT_LINK:
		indicator = '@';
		break;
	case FMT_SOCK:
		indicator = '=';
		break;
	case FMT_FIFO:
		indicator = '|';
		break;
	case FMT_EXE:
		indicator = '*';
		break;
	}
	if (!skip_fmt)
		memset(buf + strlen(p), indicator, 1);
	if (fmt_flag == FMT_LINK) {
		char sprintf_buf[DIRSIZ];
		char readlink_buf[DIRSIZ];
		if (readlink(buf, readlink_buf, DIRSIZ) < 0) {
			perror("readlink");
			exit(1);
		}
		sprintf(sprintf_buf, "%s%c -> %s", p, indicator, readlink_buf);
		memcpy(buf, sprintf_buf, DIRSIZ);
	}

	return buf;
}

static char *
mode_to_perm(uint mode, char ret[static 11])
{
	ret[0] = (mode & S_IFREG) ? '-' :
					 S_ISDIR(mode)		? 'd' :
					 S_ISBLK(mode)		? 'c' :
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
ls_format(char *buf, struct stat st, bool pflag, bool lflag, bool hflag,
					bool iflag)
{
	int fmt_ret = 0;
	char human_bytes_buf[12];
	if (pflag) {
		switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			fmt_ret = FMT_DIR;
			break;
		case S_IFLNK:
			fmt_ret = FMT_LINK;
			break;
		default:
			break;
		}
	}
	if (lflag) {
		// inodes
		if (iflag)
			fprintf(stdout, "% 9d ", st.st_ino);
		char ret[11];
		fprintf(stdout, "%s ", mode_to_perm(st.st_mode, ret));
		if (hflag)
			fprintf(stdout, "% 4d % 4d %s ", st.st_uid, st.st_gid,
							to_human_bytes(st.st_size, human_bytes_buf));
		else
			fprintf(stdout, "% 4d % 4d % 6d ", st.st_uid, st.st_gid, st.st_size);
		struct ls_time lt = unix_time_to_human_readable(st.st_mtime);
		fprintf(stdout, "%04d-%02d-%02d %02d:%02d:%02d ", lt.yr, lt.mo, lt.day,
						lt.hr, lt.min, lt.sec);


		if (S_ISLNK(st.st_mode)) {
			fprintf(stdout, "%s\n", fmtname(buf, true));
		} else {
			fprintf(stdout, "%s\n", fmtname(buf, pflag ? fmt_ret : 0));
		}
	} else {
		fprintf(stdout, "%s ", fmtname(buf, pflag ? fmt_ret : 0));
	}
}

// this ls(1) tries to follow __minimal__ POSIX stuff.
static void
ls(char *path, bool lflag, bool iflag, bool pflag, bool hflag)
{
	int fd;
	struct dirent de;
	struct stat st;
	char buf[512];

	if ((fd = open(path, 0)) < 0) {
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
	case S_IFREG:
		ls_format(path, st, pflag, lflag, hflag, iflag);
	case S_IFDIR: {
		if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
			fprintf(stderr, "ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		char *p = buf + strlen(buf);
		*p++ = '/';
		while (read(fd, &de, sizeof(de)) == sizeof(de)) {
			if (de.inum == 0)
				continue;
			strcpy(p, de.name);
			if (stat(buf, &st) < 0) {
				fprintf(stderr, "ls: cannot stat %s\n", buf);
				perror("stat");
				continue;
			}
			ls_format(buf, st, pflag, lflag, hflag, iflag);
		}
	} break;
	default:
		break;
	}
	fprintf(stdout, "%s", lflag ? "" : "\n");
	close(fd);
}

int
main(int argc, char *argv[])
{
	int i;
	bool lflag = false;
	bool iflag = false;
	bool pflag = false;
	bool hflag = false;
	// index for filenames or directory names
	int arg_idx = 1;

	if (argc < 2) {
		ls(".", lflag, iflag, pflag, hflag);
		return 0;
	}
	for (int j = 1; j < argc; j++) {
		if (argv[j][0] == '-' && argv[j][1] != '\0') {
			for (int k = 1; k < strlen(argv[j]); k++) {
				switch (argv[j][k]) {
				case 'l':
					lflag = true;
					break;
				case 'i':
					iflag = true;
					break;
				case 'p':
					pflag = true;
					break;
				case 'h':
					hflag = true;
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
		ls(".", lflag, iflag, pflag, hflag);
		return 0;
	}

	for (i = arg_idx; i < argc; i++) {
		struct stat unused;
		if (stat(argv[i], &unused) >= 0) {
			ls(argv[i], lflag, iflag, pflag, hflag);
		} else {
			int err_no = errno;
			printf("ls: can't open `%s'", argv[i]);
			errno = err_no;
			perror("");
		}
	}
	return 0;
}
