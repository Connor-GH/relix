#pragma once
#include <sys/types.h>
typedef unsigned int speed_t;
typedef unsigned int cc_t;
typedef unsigned int tcflag_t;

#define VEOF 0
#define VEOL 1
#define VERASE 2
#define VINTR 3
#define VKILL 4
#define VMIN 5
#define VQUIT 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VTIME 10
#define NCCS 11

#define BRKINT 0x1 // Signal interrupt on break.
#define ICRNL 0x2 // Map CR to NL on input.
#define IGNBRK 0x4 // Ignore break condition.
#define IGNCR 0x8 // Ignore CR.
#define IGNPAR 0x10 // Ignore characters with parity errors.
#define INLCR 0x20 // Map NL to CR on input.
#define INPCK 0x40 // Enable input parity check.
#define ISTRIP 0x80 // Strip character.
#define IXANY 0x100 // Enable any character to restart output.
#define IXOFF 0x200 // Enable start/stop input control.
#define IXON 0x400 // Enable start/stop output control.
#define PARMRK 0x800 // Mark parity errors.

#define OPOST 0x1 // Post-process output.
#define ONLCR 0x2 // Map NL to CR-NL on output.
#define OCRNL 0x4 // Map CR to NL on output.
#define ONOCR 0x8 // No CR output at column 0.
#define ONLRET 0x10 // NL performs CR function.
#define OFILL 0x20 // Use fill characters for delay.

#define NLDLY 0x40 // Select newline delays:
#define NL0 0x40
#define NL1 0x41

#define CRDLY 0x80 // Select carriage-return delays:
#define CR0 0x80
#define CR1 0x81
#define CR2 0x82
#define CR3 0x83

#define TABDLY 0x100 // Select horizontal-tab delays:
#define TAB0 0x100
#define TAB1 0x101
#define TAB2 0x102
#define TAB3 0x103

#define BSDLY 0x200 // Select backspace delays:
#define BS0 0x200
#define BS1 0x201

#define VTDLY 0x400 // Select vertical-tab delays:
#define VT0 0x400
#define VT1 0x401

#define FFDLY 0x800 // Select form-feed delays:
#define FF0 0x800
#define FF1 0x801

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

#define CSIZE 0x8
#define CS5 0x0
#define CS6 0x1
#define CS7 0x2
#define CS8 0x3

#define CSTOPB 0x10
#define CREAD 0x20
#define PARENB 0x40
#define PARODD 0x80
#define HUPCL 0x100
#define CLOCAL 0x200

#define ECHO 0x1 // Enable echo.
#define ECHOE 0x2 // Echo erase character as error-correcting backspace.
#define ECHOK 0x4 // Echo KILL.
#define ECHONL 0x8 // Echo NL.
#define ICANON 0x10 // Canonical input (erase and kill processing).
#define IEXTEN 0x20 // Enable extended input character processing.
#define ISIG 0x40 // Enable signals.
#define NOFLSH 0x80 // Disable flush after interrupt or quit.
#define TOSTOP 0x100 // Send SIGTTOU for background output.

// tcsetattr()
#define TCSANOW 1
#define TCSADRAIN 2
#define TCSAFLUSH 4

// tcflush()
#define TCIFLUSH 1
#define TCIOFLUSH 2
#define TCOFLUSH 4

// tcflow()
#define TCIOFF 1
#define TCION 2
#define TCOOFF 4
#define TCOON 8

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
	// POSIX.1-2024 is silent about these two,
	// but they are the easiest way to setup tc{g,s}et{o,i}speed().
	// Additionally, it seems that every system uses these anyway.
	speed_t c_ispeed;
	speed_t c_ospeed;
};

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
};

speed_t cfgetispeed(const struct termios *);
speed_t cfgetospeed(const struct termios *);
int cfsetispeed(struct termios *, speed_t);
int cfsetospeed(struct termios *, speed_t);
int tcdrain(int);
int tcflow(int, int);
int tcflush(int, int);
int tcgetattr(int, struct termios *);
pid_t tcgetsid(int);
int tcsendbreak(int, int);
int tcsetattr(int, int, const struct termios *);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);

int tcgetwinsize(int fd, struct winsize *);
int tcsetwinsize(int fd, const struct winsize *);
