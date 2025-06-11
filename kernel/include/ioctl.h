#pragma once
/* Exported to userspace */
#include "fb.h"
#include <pci.h>
#include <sys/types.h>
#include <termios.h>
#define _IOC_RW 0b11
#define _IOC_RO 0b01
#define _IOC_WO 0b10
#define _IOC_NONE 0
// 8-bit magic, 2 bit rw, 14 bit size, 8 bit number (for that magic)
// 8 + 2 + 14 + 8 = 32 bits
// In other words, 255 different drivers, 255 different subcommands for each
// driver. In the future, this constant may change.
#define _IOC(drv_magic, rw, size, number) \
	(unsigned long)((drv_magic << 24) | (number << 16) | (size << 2) | rw)

// PCI ioctls.
#define PCIIOCGETCONF _IOC('P', _IOC_RW, sizeof(struct pci_conf), 0)

// Framebuffer (/dev/fb0).
#define FBIOCGET_VSCREENINFO \
	_IOC('F', _IOC_RW, sizeof(struct fb_var_screeninfo), 0)

// Termios
// Get attributes.
#define TIOCGETA _IOC('T', _IOC_RW, sizeof(struct termios), 0)
#define TIOCGETAW _IOC('T', _IOC_RW, sizeof(struct termios), 1)
#define TIOCGETAF _IOC('T', _IOC_RW, sizeof(struct termios), 2)

// Set attributes.
#define TIOCSETA _IOC('T', _IOC_RW, sizeof(struct termios), 3)
#define TIOCSETAW _IOC('T', _IOC_RW, sizeof(struct termios), 4)
#define TIOCSETAF _IOC('T', _IOC_RW, sizeof(struct termios), 5)

// Set/get the p-group.
#define TIOCSPGRP _IOC('T', _IOC_RW, sizeof(pid_t), 6)
#define TIOCGPGRP _IOC('T', _IOC_RW, sizeof(pid_t), 7)

// Set/get the session id.
#define TIOCGSID _IOC('T', _IOC_RW, sizeof(pid_t), 8)
#define TIOCSSID _IOC('T', _IOC_RW, sizeof(pid_t), 9)

// Set/clear break.
#define TIOCSBRK _IOC('T', _IOC_RW, sizeof(void), 10)
#define TIOCCBRK _IOC('T', _IOC_RW, sizeof(void), 11)

#define TIOCDRAIN _IOC('T', _IOC_RW, sizeof(void), 12)
#define TIOCFLUSH _IOC('T', _IOC_RW, sizeof(int), 13)
#define TIOCSTOP _IOC('T', _IOC_RW, sizeof(void), 14)
#define TIOCSTART _IOC('T', _IOC_RW, sizeof(void), 15)

#define TIOCSWINSZ _IOC('T', _IOC_RW, sizeof(struct winsize), 16)
#define TIOCGWINSZ _IOC('T', _IOC_RW, sizeof(struct winsize), 17)

// Set/clear the current tty.
#define TIOCSCTTY _IOC('T', _IOC_RW, sizeof(void), 18)
#define TIOCNOTTY _IOC('T', _IOC_RW, sizeof(void), 19)
