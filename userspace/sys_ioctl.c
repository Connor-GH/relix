#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include "libc_syscalls.h"

int
ioctl(int fd, unsigned long request, ...)
{
	void *arg;
	va_list listp;
	switch (request) {
	case PCIIOCGETCONF:
	case FBIOCGET_VSCREENINFO:
	case TIOCGETAW:
	case TIOCSETAW:
	case TIOCGETAF:
	case TIOCSETAF:
	case TIOCGETA:
	case TIOCSETA:
	case TIOCGPGRP:
	case TIOCSPGRP:
	case TIOCGSID:
	case TIOCSSID:
	case TIOCFLUSH:
	case TIOCGWINSZ:
	case TIOCSWINSZ:
		va_start(listp, request);
		arg = va_arg(listp, void *);
		va_end(listp);
		break;
	case TIOCSBRK:
	case TIOCCBRK:
	case TIOCDRAIN:
	case TIOCSTOP:
	case TIOCSTART:
	case TIOCSCTTY:
	case TIOCNOTTY:
	default:
		arg = NULL;
		break;
	}
	return __syscall_ret(__syscall3(SYS_ioctl, fd, request, (long)arg));
}
