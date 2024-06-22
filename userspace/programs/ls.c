#include <sys/stat.h>
#include <kernel/include/fs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

static char *
disambiguate_symlink(uint inode, char *path)
{
	char *p;
	for (p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	// name is now top level
	p++;
	struct stat st;
	(void)path;
	(void)stat("/" /*path*/, &st);
	if (S_ISDIR(st.st_mode)) {
		// idk;
	}
	return "idk!";
}

struct ls_time {
	uint sec;
	uint min;
	uint hr;
	uint day;
	uint mo;
	uint yr;
};
static struct ls_time
to_human_time(uint time)
{
	struct ls_time lt;
	lt.sec = time;
	lt.min = lt.sec / 60;
	lt.hr = lt.min / 60;
	lt.day = lt.hr / 23.981777;
	lt.mo = lt.day / 30.44;
	lt.yr = lt.day / 365.25;
	lt.yr += 1970;
	lt.mo %= 12;
	lt.day %= 30;
	lt.hr = (lt.hr % 24);
	lt.min %= 60;
	lt.sec %= 60;
	return lt;
}

static char *
fmtname(char *path, bool pflag)
{
	static char buf[DIRSIZ + 1];
	char *p;
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
	if (pflag)
		memset(buf + strlen(p), '/', 1);
	memset(buf + strlen(p) + pflag, ' ', 1);
	return buf;
}

static char *
fmtname_file(char *path)
{
	return fmtname(path, false);
}

static char *
mode_to_perm(uint mode, char *ret /*[11]*/)
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
// this ls(1) tries to follow __minimal__ POSIX stuff.
void
ls(char *path, bool lflag, bool iflag, bool pflag)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if ((fd = open(path, 0)) < 0) {
		fprintf(stderr, "ls: cannot open %s\n", path);
		return;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch (st.st_mode & S_IFMT) {
	case S_IFREG:
		if (lflag) {
			// inodes
			if (iflag)
				fprintf(stdout, "% 9d ", st.st_ino);
			char ret[11];
			fprintf(stdout, "%s ", mode_to_perm(st.st_mode, ret));
			fprintf(stdout, "% 4d % 4d % 9d ", st.st_uid, st.st_gid, st.st_size);
			struct ls_time lt = to_human_time(st.st_mtime);
			fprintf(stdout, "%04d-%02d-%02d %02d:%02d:%02d ", lt.yr, lt.mo, lt.day,
							lt.hr, lt.min, lt.sec);
			fprintf(stdout, "%s ", fmtname_file(path));
			if (st.st_nlink > 1) {
				fprintf(stdout, "-> %s", disambiguate_symlink(st.st_ino, path));
			}
		} else {
			fprintf(stdout, "%s", fmtname_file(path));
		}
		fprintf(stdout, "\n");
		break;

	case S_IFDIR: {
		if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
			fprintf(stdout, "ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		p = buf + strlen(buf);
		*p++ = '/';
		while (read(fd, &de, sizeof(de)) == sizeof(de)) {
			if (de.inum == 0)
				continue;
			strcpy(p, de.name);
			if (stat(buf, &st) < 0) {
				fprintf(stdout, "ls: cannot stat %s\n", buf);
				continue;
			}
			if (lflag) {
				// inodes
				if (iflag)
					fprintf(stdout, "% 9d ", st.st_ino);
				char ret[11];
				fprintf(stdout, "%s ", mode_to_perm(st.st_mode, ret));
				fprintf(stdout, "% 4d % 4d % 9d ", st.st_uid, st.st_gid, st.st_size);
				struct ls_time lt = to_human_time(st.st_mtime);
				fprintf(stdout, "%04d-%02d-%02d %02d:%02d:%02d ", lt.yr, lt.mo, lt.day,
								lt.hr, lt.min, lt.sec);
				fprintf(stdout, "%s", fmtname(buf, pflag && S_ISDIR(st.st_mode)));
				if (st.st_nlink > 1)
					fprintf(stdout, "-> %s", "TODO");
				fprintf(stdout, "\n");
			} else {
				fprintf(stdout, "%s", fmtname(buf, pflag && S_ISDIR(st.st_mode)));
			}
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
	// index for filenames or directory names
	int arg_idx = 1;

	if (argc < 2) {
		ls(".", lflag, iflag, pflag);
		exit(0);
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
		ls(".", lflag, iflag, pflag);
		exit(0);
	}

	for (i = 1; i < argc; i++) {
		struct stat unused;
		if (stat(argv[i], &unused) >= 0) {
			ls(argv[i], lflag, iflag, pflag);
		}
	}
	exit(0);
}
