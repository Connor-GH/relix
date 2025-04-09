#pragma once
#include <sys/types.h>
typedef unsigned int speed_t;
typedef unsigned int cc_t;
typedef unsigned int tcflag_t;

enum __termios_nccs {
	VEOF = 0,
	VEOL,
	VERASE,
	VINTR,
	VKILL,
	VMIN,
	VQUIT,
	VSTART,
	VSTOP,
	VSUSP,
	VTIME,
	NCCS,
};

enum __termios_c_iflag {
	BRKINT = 0x1, // Signal interrupt on break.
	ICRNL = 0x2, // Map CR to NL on input.
	IGNBRK = 0x4, // Ignore break condition.
	IGNCR = 0x8, // Ignore CR.
	IGNPAR = 0x10, // Ignore characters with parity errors.
	INLCR = 0x20, // Map NL to CR on input.
	INPCK = 0x40, // Enable input parity check.
	ISTRIP = 0x80, // Strip character.
	IXANY = 0x100, // Enable any character to restart output.
	IXOFF = 0x200, // Enable start/stop input control.
	IXON = 0x400, // Enable start/stop output control.
	PARMRK = 0x800, // Mark parity errors.
};

enum __termios_c_oflag {
	OPOST = 0x1, // Post-process output.
	ONLCR = 0x2, // Map NL to CR-NL on output.
	OCRNL = 0x4, // Map CR to NL on output.
	ONOCR = 0x8, // No CR output at column 0.
	ONLRET = 0x10, // NL performs CR function.
	OFILL = 0x20, // Use fill characters for delay.
	NLDLY = 0x40, // Select newline delays:
#define NL0 0x40
#define NL1 0x41
	CRDLY = 0x80, // Select carriage-return delays:
#define CR0 0x80
#define CR1 0x81
#define CR2 0x82
#define CR3 0x83
	TABDLY = 0x100, // Select horizontal-tab delays:
#define TAB0 0x100
#define TAB1 0x101
#define TAB2 0x102
#define TAB3 0x103
	BSDLY = 0x200, // Select backspace delays:
#define BS0 0x200
#define BS1 0x201
	VTDLY = 0x400, // Select vertical-tab delays:
#define VT0 0x400
#define VT1 0x401
	FFDLY = 0x800, // Select form-feed delays:
#define FF0 0x800
#define FF1 0x801
};

// Baud rates.
#define B0 0
#define B50 1
#define B75 2
#define B110 3
#define B134 4
#define B150 5
#define B200 6
#define B300 7
#define B600 8
#define B1200 9
#define B1800 10
#define B2400 11
#define B4800 12
#define B9600 13
#define B19200 14
#define B38400 15

enum __termios_c_cflag {
	CSIZE = 0x8,
#define CS5 0x0
#define CS6 0x1
#define CS7 0x2
#define CS8 0x3
	CSTOPB = 0x10,
	CREAD = 0x20,
	PARENB = 0x40,
	PARODD = 0x80,
	HUPCL = 0x100,
	CLOCAL = 0x200,
};

enum __termios_c_lflag {
	ECHO = 0x1, // Enable echo.
	ECHOE = 0x2, // Echo erase character as error-correcting backspace.
	ECHOK = 0x4, // Echo KILL.
	ECHONL = 0x8, // Echo NL.
	ICANON = 0x10, // Canonical input (erase and kill processing).
	IEXTEN = 0x20, // Enable extended input character processing.
	ISIG = 0x40, // Enable signals.
	NOFLSH = 0x80, // Disable flush after interrupt or quit.
	TOSTOP = 0x100, // Send SIGTTOU for background output.
};

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
};

speed_t
cfgetispeed(const struct termios *);
speed_t
cfgetospeed(const struct termios *);
int
cfsetispeed(struct termios *, speed_t);
int
cfsetospeed(struct termios *, speed_t);
int
tcdrain(int);
int
tcflow(int, int);
int
tcflush(int, int);
int
tcgetattr(int, struct termios *);
pid_t
tcgetsid(int);
int
tcsendbreak(int, int);
int
tcsetattr(int, int, const struct termios *);
