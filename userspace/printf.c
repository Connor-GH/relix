#include "../include/types.h"
#include "include/user.h"
#include "stdarg.h"
#include "../include/stdio.h"
#include "../include/stdbool.h"

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

enum {
  FLAG_PADZERO = 0x10,
  FLAG_ALTFORM = 0x20,
  FLAG_RJUST = 0x30,
  FLAG_BLANK = 0x40,
  FLAG_SIGN = 0x50,
};
#define IS_SET(x, flag) (bool)((x & flag) == flag)

static void
printint(int fd, int xx, int base, bool sgn, int flags, int padding)
{
  static char digits[] = "0123456789ABCDEF";
  char buf[250];
  int i = 0;
  int neg = 0;
  uint x;

  if (sgn && xx < 0) {
	  neg = 1;
	  x = -xx;
  } else {
	  x = xx;
  }

  do {
	  buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  // pad for zeroes (and blanks)
  if (IS_SET(flags, FLAG_PADZERO)) {
    while (i < padding-1) {
      buf[i++] = '0';
    }
  }
  if (IS_SET(flags, FLAG_BLANK)) {
    while (i < padding-1) {
      buf[i++] = ' ';
    }
  }
  if (base == 16 && IS_SET(flags, FLAG_ALTFORM)) {
    buf[i++] = 'x';
    buf[i++] = '0';
  }
  // append negative/positive sign
  if (neg)
	  buf[i++] = '-';
  else if (IS_SET(flags, FLAG_SIGN))
    buf[i++] = '+';

  while (--i >= 0)
	  putc(fd, buf[i]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
vfprintf(int fd, const char *fmt, va_list *argp)
{
  char *s;
  int c, i, state;
  int flags = 0;
  int str_pad = 16;

  state = 0;
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
      switch (c) {
      case '0':
        flags |= FLAG_PADZERO;
        goto skip_state_reset;
        break;
      case '#':
        flags |= FLAG_ALTFORM;
        goto skip_state_reset;
        break;
      case '-':
        flags |= FLAG_RJUST;
        if (fmt[i+1] && (fmt[i+1] - '0' >= 0)) {
          str_pad = fmt[++i] - '0';
        }
        goto skip_state_reset;
        break;
      case ' ':
        flags |= FLAG_BLANK;
        if (fmt[i+1] && (fmt[i+1] - '0' >= 0)) {
          str_pad = fmt[++i] - '0';
        }
        goto skip_state_reset;
        break;
      case '+':
        flags |= FLAG_SIGN;
        goto skip_state_reset;
      case 'i':
      case 'd': {
        int d = va_arg(*argp, int);
        printint(fd, d, 10, true, flags, str_pad);
        break;
      }
      case 'u': {
        uint u = va_arg(*argp, uint);
        printint(fd, u, 10, false, flags, str_pad);
        break;
      }
      case 'x':
      case 'p': {
        int x = va_arg(*argp, int);
        printint(fd, x, 16, false, flags, str_pad);
        break;
      }
      case 'o': {
        int x = va_arg(*argp, int);
        printint(fd, x, 8, false, flags, str_pad);
        break;
      }
      case 's': {
        s = va_arg(*argp, char *);
        if (s == 0)
          s = "(null)";
        if (IS_SET(flags, FLAG_RJUST) && strlen(s) < str_pad) {
            for (int i = 0; i < str_pad-strlen(s); i++)
              putc(fd, ' ');
          }
          while (*s != 0) {
          putc(fd, *s);
          s++;
        }
        break;
      }
      case 'c': {
        int c = va_arg(*argp, int);
        putc(fd, c);
        break;
      }
      case '%':
        putc(fd, c);
        break;
      // Unknown % sequence.  Print it to draw attention.
      default:
        putc(fd, '%');
        putc(fd, c);
        break;
      }
	  state = 0;
    skip_state_reset: ; // state = '%' if set
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
