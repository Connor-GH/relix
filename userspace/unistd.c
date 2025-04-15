#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>

static char ttyname_buf[FILENAME_MAX];

char *
ttyname(int fd)
{
	int ret = ttyname_r(fd, ttyname_buf, sizeof(ttyname_buf));
	if (ret < 0)
		return NULL;
	return ttyname_buf;
}

int
ttyname_r(int fd, char *buf, size_t buflen)
{
	struct stat fd_stat;
	// Stat the fd to get the inode number.
	if (fstat(fd, &fd_stat) < 0) {
		return -1;
	}
	// Go through /dev to find devices (for tty).
	DIR *dir = opendir("/dev");
	if (dir == NULL)
		return -1;
	struct dirent *d;
	while ((d = readdir(dir)) != NULL) {
		// We found the right file. Put its pathname in buf.
		if (d->d_ino == fd_stat.st_ino) {
			snprintf(buf, buflen, "%s/%s", "/dev", d->d_name);
			closedir(dir);
			return 0;
		}
	}
	closedir(dir);
	return -1;
}
