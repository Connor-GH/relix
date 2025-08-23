#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

int
tcgetattr(int fd, struct termios *t)
{
	return ioctl(fd, TIOCGETA, t);
}

int
tcsetattr(int fd, int opt, const struct termios *t)
{
	struct termios localterm;

	switch (opt) {
	case TCSANOW:
		return ioctl(fd, TIOCSETA, t);
	case TCSADRAIN:
		return ioctl(fd, TIOCSETAW, t);
	case TCSAFLUSH:
		return ioctl(fd, TIOCSETAF, t);
	default:
		errno = EINVAL;
		return -1;
	}
}

int
tcsetpgrp(int fd, pid_t pgrp)
{
	return ioctl(fd, TIOCSPGRP, pgrp);
}

pid_t
tcgetpgrp(int fd)
{
	int pgid;

	if (ioctl(fd, TIOCGPGRP, &pgid) < 0) {
		return (pid_t)-1;
	}

	return (pid_t)pgid;
}

pid_t
tcgetsid(int fd)
{
	int s;

	if (ioctl(fd, TIOCGSID, &s) < 0) {
		return (pid_t)-1;
	}

	return (pid_t)s;
}

speed_t
cfgetospeed(const struct termios *t)
{
	return t->c_ospeed;
}

speed_t
cfgetispeed(const struct termios *t)
{
	return t->c_ispeed;
}

int
cfsetospeed(struct termios *t, speed_t speed)
{
	t->c_ospeed = speed;
	return 0;
}

int
cfsetispeed(struct termios *t, speed_t speed)
{
	t->c_ispeed = speed;
	return 0;
}

// Nonstandard helper w.r.t. POSIX.
void
cfmakeraw(struct termios *t)
{
	t->c_iflag &= ~(IXOFF | INPCK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
	                ICRNL | IXON | IGNPAR);
	t->c_iflag |= IGNBRK;
	t->c_oflag &= ~OPOST;
	t->c_lflag &=
		~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | ISIG | IEXTEN | NOFLSH | TOSTOP);
	t->c_cflag &= ~(CSIZE | PARENB);
	t->c_cflag |= CS8 | CREAD;
	t->c_cc[VMIN] = 1;
	t->c_cc[VTIME] = 0;
}

int
tcsendbreak(int fd, int len)
{
	struct timeval sleepytime;

	sleepytime.tv_sec = 0;
	sleepytime.tv_usec = 400000;
	if (ioctl(fd, TIOCSBRK) == -1) {
		return -1;
	}
	// TODO:
	// The BSD implementation puts this here
	// in order to wait for the terminal to
	// become ready. When we implement select(2),
	// this should be uncommented.
#if 0
	(void)select(0, 0, 0, 0, &sleepytime);
#endif
	if (ioctl(fd, TIOCCBRK) == -1) {
		return -1;
	}
	return 0;
}

int
tcdrain(int fd)
{
	return ioctl(fd, TIOCDRAIN, 0);
}

int
tcflush(int fd, int which)
{
	// The kernel will decipher the type of flush to be done.
	return ioctl(fd, TIOCFLUSH, &which);
}

int
tcflow(int fd, int action)
{
	struct termios term;
	unsigned char c;

	switch (action) {
	case TCOOFF:
		return ioctl(fd, TIOCSTOP, 0);
	case TCOON:
		return ioctl(fd, TIOCSTART, 0);
	case TCION:
	case TCIOFF:
		if (tcgetattr(fd, &term) == -1) {
			return -1;
		}
		c = term.c_cc[action == TCIOFF ? VSTOP : VSTART];
		if (c != _POSIX_VDISABLE && write(fd, &c, sizeof(c)) == -1) {
			return -1;
		}
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}

int
tcgetwinsize(int fd, struct winsize *w)
{
	return ioctl(fd, TIOCGWINSZ, w);
}

int
tcsetwinsize(int fd, const struct winsize *w)
{
	return ioctl(fd, TIOCSWINSZ, w);
}
