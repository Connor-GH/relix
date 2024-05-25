#include "types.h"
#include "stat.h"
#include "user.h"
#include "stdarg.h"
#include "stdio.h"

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static void
printint(int fd, int xx, int base, int sgn)
{
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if (sgn && xx < 0) {
	neg = 1;
	x = -xx;
  } else {
	x = xx;
  }

  i = 0;
  do {
	buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  if (neg)
	buf[i++] = '-';

  while (--i >= 0)
	putc(fd, buf[i]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
vfprintf(int fd, const char *fmt, va_list *argp)
{
  char *s;
  int c, i, state;
  //uint *ap;

  state = 0;
  //ap = (uint*)(void*)&fmt + 1;
  for (i = 0; fmt[i]; i++) {
	// 'floor' character down to bottom 255 chars
	c = fmt[i] & 0xff;
	if (state == 0) {
	  if (c == '%') {
		state = '%';
	  } else {
		putc(fd, c);
	  }
	} else if (state == '%') {
	  if (c == 'd') {
		int d = va_arg(*argp, int);
		printint(fd, d, 10, 1);
	  } else if (c == 'x' || c == 'p') {
		int x = va_arg(*argp, int);
		printint(fd, x, 16, 0);
	  } else if (c == 's') {
		s = va_arg(*argp, char *);
		if (s == 0)
		  s = "(null)";
		while (*s != 0) {
		  putc(fd, *s);
		  s++;
		}
	  } else if (c == 'c') {
		int c = va_arg(*argp, int);
		putc(fd, c);
	  } else if (c == '%') {
		putc(fd, c);
	  } else {
		// Unknown % sequence.  Print it to draw attention.
		putc(fd, '%');
		putc(fd, c);
	  }
	  state = 0;
	}
  }
}

void
fprintf(int fd, const char *fmt, ...)
{
  va_list listp;
  va_start(listp, fmt);
  vfprintf(fd, fmt, &listp);
  va_end(listp);
}

void
printf(const char *fmt, ...)
{
  va_list listp;
  va_start(listp, fmt);
  vfprintf(stdout, fmt, &listp);
  va_end(listp);
}
