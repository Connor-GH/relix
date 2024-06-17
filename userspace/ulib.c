#include <types.h>
#include <stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <user.h>
#include <kernel/include/x86.h>

char *
strcpy(char *s, const char *t)
{
	char *os;

	os = s;
	while ((*s++ = *t++) != 0)
		;
	return os;
}

char *
strcat(char *dst, const char *src)
{
  int start = strlen(dst);
  int j = 0;
  for (int i = start; i < start + strlen(src) + 1; i++, j++) {
    dst[i] = src[j];
  }
  return dst;
}

int
strcmp(const char *p, const char *q)
{
	while (*p && *p == *q)
		p++, q++;
	return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
	int n;

	for (n = 0; s[n]; n++)
		;
	return n;
}

void *
memset(void *dst, int c, uint n)
{
	stosb(dst, c, n);
	return dst;
}

char *
strchr(const char *s, char c)
{
	for (; *s; s++)
		if (*s == c)
			return (char *)s;
	return 0;
}

int
getc(FILE fd)
{
  int c;
  read(fd, &c, 1);
  return c;
}

char *
gets(char *buf, int max)
{
	int i;
	char c;

	for (i = 0; i + 1 < max;) {
		c = getc(stdin);
		buf[i++] = c;
		if (c == '\n' || c == '\r')
			break;
	}
	buf[i] = '\0';
	return buf;
}

// fill in st from pathname n
int
stat(const char *n, struct stat *st)
{
	int fd;
	int r;

	fd = open(n, O_RDONLY);
	if (fd < 0)
		return -1;
	r = fstat(fd, st);
	close(fd);
	return r;
}

DIR *
fopendir(int fd)
{
	if (fd == -1)
		return NULL;
	DIR *dirp = (DIR *)malloc(sizeof(DIR));
	dirp->fd = fd;
	dirp->buffer = NULL;
	dirp->buffer_size = 0;
	dirp->nextptr = NULL;
	return dirp;
}

DIR *
opendir(const char *path)
{
  int fd = open(path, O_RDONLY /*| O_DIRECTORY*/);
  if (fd == -1)
    return NULL;
  return fopendir(fd);
}
int closedir(DIR *dir) {
  if (dir == NULL || dir->fd == -1) {
    return -1; // EBADF
  }
  if (dir->buffer != NULL)
    free(dir->buffer);
  int rc = close(dir->fd);
  if (rc == 0)
    dir->fd = -1;
  free(dir);
  return rc;
}

// no way to check for error...sigh....
// atoi("0") == 0
// atoi("V") == 0
// "V" != "0"
int
atoi(const char *s)
{
	int n = 0;

	while ('0' <= *s && *s <= '9')
		n = n * 10 + *s++ - '0';
	return n;
}
int
atoi_base(const char *s, uint base)
{
	int n = 0;

	if (base <= 10) {
		while ('0' <= *s && *s <= '9')
			n = n * base + *s++ - '0';
		return n;
	} else if (base <= 16) {
		while (('0' <= *s && *s <= '9') || ('a' <= *s && *s <= 'f')) {
			if ('0' <= *s && *s <= '9')
				n = n * base + *s++ - '0';
			if ('a' <= *s && *s <= 'f')
				n = n * base + *s++ - 'a';
		}
		return n;
	} else {
		return 0;
	}
}

void *
memmove(void *vdst, const void *vsrc, uint n)
{
	char *dst;
	const char *src;

	dst = vdst;
	src = vsrc;
	while (n-- > 0)
		*dst++ = *src++;
	return vdst;
}

void
assert_fail(const char *assertion, const char *file, int lineno,
						const char *func)
{
	fprintf(stderr, "%s:%d: %s: Assertion `%s' failed.\n", file, lineno, func,
					assertion);
  fprintf(stderr, "Aborting.\n");
	exit(-1);
}
