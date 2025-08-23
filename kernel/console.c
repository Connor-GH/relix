// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "console.h"
#include "boot/multiboot2.h"
#include "drivers/lapic.h"
#include "file.h"
#include "kbd.h"
#include "lib/compiler_attributes.h"
#include "lib/queue.h"
#include "macros.h"
#include "mman.h"
#include "proc.h"
#include "spinlock.h"
#include "symbols.h"
#include "termios.h"
#include "uart.h"
#include "vga.h"
#include "x86.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern size_t let_rust_handle_it(const char *fmt);

int echo_out = 1;
static int panicked = 0;
static uint32_t static_foreg = VGA_COLOR_WHITE;
static uint32_t static_backg = VGA_COLOR_BLACK;
static int alt_form = 0;
static int long_form = 0;
static int zero_form = 0;
static int active_term = 0;

typedef void (*putfunc_t)(int, uint32_t, uint32_t);
__nonnull(1) static void vcprintf(putfunc_t putfunc, const char *fmt,
                                  va_list argp, int locking);
static void consputc3(int c, uint32_t foreg, uint32_t backg);
/*
 * This resource protects any static variable in this file, but mainly:
 * - console_buffer
 * - buffer_position
 */
static struct {
	struct spinlock lock;
	int locking;
} cons;

static struct termios tty_settings[NTTY] = { 0 };
static pid_t term_pgids[NTTY] = {};
// This should always be in bounds, provided that
// only the minor from ip->minor is passed in, and
// that ip->major == TTY.
struct termios *
get_term_settings(int minor)
{
	return &tty_settings[minor];
}

pid_t
get_term_pgid(int minor)
{
	return term_pgids[minor];
}

void
set_term_pgid(int minor, pid_t pgid)
{
	term_pgids[minor] = pgid;
}

int
get_active_term(void)
{
	return active_term;
}

void
set_active_term(int minor)
{
	active_term = minor;
}

void
set_term_settings(int minor, struct termios *termios)
{
	struct termios local = {
		.c_lflag = termios->c_lflag,
		.c_cflag = termios->c_cflag,
		.c_iflag = termios->c_iflag,
		.c_oflag = termios->c_oflag,
		.c_ispeed = termios->c_ispeed,
		.c_ospeed = termios->c_ospeed,
	};
	for (int i = 0; i < NCCS; i++) {
		local.c_cc[i] = termios->c_cc[i];
	}

	tty_settings[minor] = local;
}

// color is default if it is set to 0xff
static void
set_term_color(uint32_t foreground, uint32_t background, bool changed_fg,
               bool changed_bg)
{
	if (foreground == 0 && background == 0) {
		static_foreg = VGA_COLOR_WHITE;
		static_backg = VGA_COLOR_BLACK;
	} else {
		static_foreg = (!changed_fg) ? static_foreg : foreground;
		static_backg = (!changed_bg) ? static_backg : background;
	}
}

static void
uartputc3(int c, uint32_t unused, uint32_t unused2)
{
	uartputc(c);
}

uint32_t
ansi_4bit_to_hex_color(uint16_t color, bool is_background)
{
	if (is_background) {
		// Background colors start at 40 instead of 30.
		color = saturating_sub(color, 10, 0);
	}
	switch (color) {
	case 0: {
		static_foreg = VGA_COLOR_WHITE;
		static_backg = VGA_COLOR_BLACK;
		return 0;
	}
	case 30:
		return VGA_COLOR_BLACK;
	case 31:
		return VGA_COLOR_RED;
	case 32:
		return VGA_COLOR_GREEN;
	case 33:
		return VGA_COLOR_YELLOW;
	case 34:
		return VGA_COLOR_BLUE;
	case 35:
		return VGA_COLOR_MAGENTA;
	case 36:
		return VGA_COLOR_CYAN;
	case 37:
		return VGA_COLOR_WHITE;
	case 90:
		return VGA_COLOR_BRIGHT_BLACK;
	case 91:
		return VGA_COLOR_BRIGHT_RED;
	case 92:
		return VGA_COLOR_BRIGHT_GREEN;
	case 93:
		return VGA_COLOR_BRIGHT_YELLOW;
	case 94:
		return VGA_COLOR_BRIGHT_BLUE;
	case 95:
		return VGA_COLOR_BRIGHT_MAGENTA;
	case 96:
		return VGA_COLOR_BRIGHT_CYAN;
	case 97:
		return VGA_COLOR_BRIGHT_WHITE;
	default:
		return VGA_COLOR_WHITE;
	}
}
void
ansi_change_color(bool bold, uint32_t color, uint8_t c, bool fg)
{
	// All attributes, [0, 29].
	if (color <= 29) {
		return;
	}
	if (fg) {
		set_term_color(color, 0, true, false);
	} else {
		set_term_color(0, color, false, true);
	}
}
static void
uartputc_wrapper(char c, char *buf)
{
	uartputc3(c, 0, 0);
}
size_t
ansi_noop(const char *s)
{
	return 0;
}
extern int early_init;

__attribute__((format(printf, 1, 2)))
__nonnull(1) void uart_printf(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	kernel_vprintf_template(uartputc_wrapper, NULL, NULL, fmt, argp, &cons.lock,
	                        ncpu > 1 && !early_init, -1);
	va_end(argp);
}

// For the case where we need to debug inside of something that may hold
// cons.lock.
__attribute__((format(printf, 1, 2)))
__nonnull(1) void uart_printf_unlocked(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	kernel_vprintf_template(uartputc_wrapper, NULL, NULL, fmt, argp, NULL, false,
	                        -1);
	va_end(argp);
}

static void
vga_write_char_wrapper(char c, char *buf)
{
	vga_write_char(c, static_foreg, static_backg);
}

__attribute__((deprecated("Use vga_cprintf or uart_printf")))
__attribute__((format(printf, 1, 2))) __nonnull(1) void cprintf(const char *fmt,
                                                                ...)
{
	va_list argp;
	va_start(argp, fmt);
	kernel_vprintf_template(vga_write_char_wrapper, let_rust_handle_it, NULL, fmt,
	                        argp, &cons.lock, true, -1);
	va_end(argp);
}
__attribute__((format(printf, 1, 2)))
__nonnull(1) void vga_cprintf(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	kernel_vprintf_template(vga_write_char_wrapper, let_rust_handle_it, NULL, fmt,
	                        argp, &cons.lock, true, -1);
	va_end(argp);
}
size_t global_string_index = 0;
static void
string_putc_wrapper(char c, char *buf)
{
	buf[global_string_index++] = c;
}

// We explicitly do ansi_noop because we want to skip over escape sequences.
__attribute__((format(printf, 2, 3)))
__nonnull(1) void ksprintf(char *restrict str, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	global_string_index = 0;
	kernel_vprintf_template(string_putc_wrapper, ansi_noop, str, fmt, argp,
	                        &cons.lock, true, -1);
	va_end(argp);
}

__cold void
panic_print_before(void)
{
	cli();
	cons.locking = 1;
	vga_reset_char_index();
	// use lapiccpunum so that we can call panic from mycpu()
	vga_cprintf("\033[1;31mlapicid %d: panic: ", lapicid());
}

__noreturn __cold void
panic_print_after(void)
{
	uintptr_t pcs[10];
	getcallerpcs(pcs);
	vga_cprintf("\nStack frames:\n");
	for (int i = 0; i < 10; i++) {
		// The relative positition within function.
		size_t relative_pos;
		const char *name = symbol_resolve(pcs[i], &relative_pos);

		if (name != NULL && relative_pos != 0) {
			vga_cprintf("%s+%#lx\n", name, relative_pos);
		}
	}
	vga_cprintf("\033[0m");
	panicked = 1; // freeze other CPU
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wanalyzer-infinite-loop"
#endif
	for (;;)
		;
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4

// Cursor position: col + 80*row.
// TODO: cursor_position currently does nothing. We should change that soon.
static int
cursor_position(void)
{
	int pos;
	outb(CRTPORT, 14);
	pos = inb(CRTPORT + 1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT + 1);
	return pos;
}

void
consputc(int c)
{
	consputc3(c, static_foreg, static_backg);
}
static void
consputc3(int c, uint32_t foreg, uint32_t backg)
{
	if (c == BACKSPACE) {
		uartputc('\b');
		uartputc(' ');
		uartputc('\b');
	} else {
		uartputc(c);
	}
	if (echo_out) {
		vga_write_char(c, static_foreg, static_backg);
	}
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint32_t r; // Read index
	uint32_t w; // Write index
	uint32_t e; // Edit index
} input;

#define C(x) ((x) - '@') // Control-x

static void
put_ctrl_char(int c)
{
	vga_write_char('^', static_foreg, static_backg);
	vga_write_char(c, static_foreg, static_backg);
}

void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;
	bool do_ctrl_c = false;
	if (panicked) {
		cli();
		for (;;)
			;
	}

	acquire(&cons.lock);
	while ((c = getc()) >= 0) {
		if (kbd_enqueue(c) == QUEUE_OOM) {
			panic("Cannot allocate memory for kbd queue");
		}
		c = kbd_scancode_into_char(c);
		switch (c) {
		case C('P'): // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			doprocdump = 1;
			break;
		case C('U'): // Kill line.
			while (input.e != input.w &&
			       input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
				input.e--;
				if (echo_out) {
					vga_write_char(BACKSPACE, static_foreg, static_backg);
				}
			}
			break;
		case C('H'):
		case '\x7f': // Backspace
			if (input.e != input.w) {
				input.e--;
				consputc3(BACKSPACE, static_foreg, static_backg);
			}
			break;
		// ETX
		case C('C'):
			do_ctrl_c = true;
			break;
		default:
			if (c != 0 && input.e - input.r < INPUT_BUF) {
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				if (c != C('D')) {
					if (echo_out) {
						vga_write_char(c, static_foreg, static_backg);
					}
				}
				if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
					if (c == C('D')) {
						if (echo_out) {
							vga_write_char('\n', static_foreg, static_backg);
						}
					}
					input.w = input.e;
					wakeup(&input.r);
				}
			}
			break;
		}
	}
	release(&cons.lock);
	if (doprocdump) {
		procdump(); // now call procdump() wo. cons.lock held
	}
	// TODO: handle all VT escape sequences and scancodes.
	if (do_ctrl_c) {
		put_ctrl_char('C');
		kill(-get_term_pgid(get_active_term()), SIGINT);
	}
}
__nonnull(2, 3) static ssize_t
	consoleread(short minor, struct inode *ip, char *dst, size_t n)
{
	size_t target;
	int c;

	inode_unlock(ip);
	target = n;
	acquire(&cons.lock);
	while (n > 0) {
		while (input.r == input.w) {
			if (myproc()->killed) {
				release(&cons.lock);
				inode_lock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}
		c = input.buf[input.r++ % INPUT_BUF];
		if (c == C('D')) { // EOF
			if (n < target) {
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if (c == '\n') {
			break;
		}
	}
	release(&cons.lock);
	inode_lock(ip);

	return target - n;
}
/*
 * INVARIANT: None of the console_{height,width}_{text,pixels} functions should
 * be called before parse_multiboot(struct multiboot_info *).
 */
int
console_width_pixels(void)
{
	return __multiboot_console_width_pixels();
}

int
console_height_pixels(void)
{
	return __multiboot_console_height_pixels();
}

/* clang-format off */
__nonnull(2, 3) static ssize_t
consolewrite(short minor, __attribute__((unused)) struct inode *ip,
																		 char *buf, size_t n)
{
	acquire(&cons.lock);
	for (size_t i = 0; i < n; i++) {
		vga_write_char(buf[i] & 0xff, static_foreg, static_backg);
	}
	release(&cons.lock);
	return n;
}
/* clang-format on */

static struct mmap_info
consolemmap_noop(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){};
}

static int
consoleopen_noop(short minor, int flags)
{
	return 0;
}

static int
consoleclose_noop(short minor)
{
	return 0;
}

__nonnull(2, 3) static ssize_t
	uartread(short minor, __attribute__((unused)) struct inode *ip, char *buf,
           size_t n)
{
	return n;
}

/* clang-format off */
__nonnull(2, 3) static ssize_t
uartwrite(short minor, __attribute__((unused)) struct inode *ip,
																		 char *buf, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		uartputc(buf[i]);
	}
	return n;
}
/* clang-format on */

static struct mmap_info
uartmmap_noop(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){};
}

static int
uartopen_noop(short minor, int flags)
{
	return 0;
}

static int
uartclose_noop(short minor)
{
	return 0;
}

__nonnull(2, 3) static ssize_t
	ttyread(short minor, __attribute__((unused)) struct inode *ip, char *buf,
          size_t n)
{
	if (minor >= MINOR_TTY_SERIAL) {
		return uartread(minor, ip, buf, n);
	} else {
		return consoleread(minor, ip, buf, n);
	}
}

/* clang-format off */
__nonnull(2, 3) static ssize_t
ttywrite(short minor, __attribute__((unused)) struct inode *ip,
																		 char *buf, size_t n)
{
	if (minor >= MINOR_TTY_SERIAL)
		return uartwrite(minor, ip, buf, n);
	else
		return consolewrite(minor, ip, buf, n);
}
/* clang-format on */

static struct mmap_info
ttymmap_noop(short minor, size_t length, uintptr_t addr, int perm)
{
	if (minor >= MINOR_TTY_SERIAL) {
		return uartmmap_noop(minor, length, addr, perm);
	} else {
		return consolemmap_noop(minor, length, addr, perm);
	}
}

static int
ttyopen_noop(short minor, int flags)
{
	if (minor >= MINOR_TTY_SERIAL) {
		return uartopen_noop(minor, flags);
	} else {
		return consoleopen_noop(minor, flags);
	}
}

static int
ttyclose_noop(short minor)
{
	if (minor >= MINOR_TTY_SERIAL) {
		return uartclose_noop(minor);
	} else {
		return consoleclose_noop(minor);
	}
}

void
consoleinit(void)
{
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	devsw[CONSOLE].mmap = consolemmap_noop;
	devsw[CONSOLE].open = consoleopen_noop;
	devsw[CONSOLE].close = consoleclose_noop;

	devsw[TTY].write = ttywrite;
	devsw[TTY].read = ttyread;
	devsw[TTY].mmap = ttymmap_noop;
	devsw[TTY].open = ttyopen_noop;
	devsw[TTY].close = ttyclose_noop;
	cons.locking = 1;
	// Notice: we start all terminals in echo mode.
	struct termios termios = {
		.c_iflag = ~(IXOFF | INPCK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
			ICRNL | IXON | IGNPAR) | IGNBRK,
		.c_oflag = ~OPOST,
		.c_lflag = ~(ECHOE | ECHOK | ECHONL | ICANON | ISIG | IEXTEN |
			NOFLSH | TOSTOP) | ECHO,
		.c_cflag = ~(CSIZE | PARENB) | CS8 | CREAD,
		.c_cc = {
			// Other elements are _POSIX_VDISABLE (0).
			[VMIN] = 1, [VTIME] = 0,
		},
		.c_ospeed = B19200,
		.c_ispeed = B19200,
	};

	// Set default term settings.
	for (int i = 0; i < NTTY; i++) {
		set_term_settings(i, &termios);
	}
}
