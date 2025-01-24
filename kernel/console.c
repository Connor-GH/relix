// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "vga.h"
#include <stdint.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "console.h"
#include "file.h"
#include "ioapic.h"
#include "boot/multiboot2.h"
#include "proc.h"
#include "spinlock.h"
#include "traps.h"
#include "uart.h"
#include "x86.h"
#include "drivers/lapic.h"
#include "compiler_attributes.h"
#include "macros.h"

extern size_t
let_rust_handle_it(const char *fmt);

static int panicked = 0;
int echo_out = 1;
static uint32_t static_foreg = VGA_COLOR_WHITE;
static uint32_t static_backg = VGA_COLOR_BLACK;
static int alt_form = 0;
static int long_form = 0;
static int zero_form = 0;

typedef void (*putfunc_t)(int, uint32_t, uint32_t);
__nonnull(1) static void vcprintf(putfunc_t putfunc, const char *fmt,
																	va_list argp);
static void
consputc3(int c, uint32_t foreg, uint32_t backg);
/*
 * This resource protects any static variable in this file, but mainly:
 * - console_buffer
 * - buffer_position
 * - crt
 */
static struct {
	struct spinlock lock;
	int locking;
} cons;

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
printint(putfunc_t putfunc, uint64_t x, int64_t xs, int base, bool is_unsigned,
				 int *padding)
{
	static char digits[] = "0123456789abcdef";
	char buf[32];
	int i;
	long_form = 0;

	i = 0;
	do {
		buf[i++] = digits[x % base];
	} while ((x /= base) != 0);
	while (i < *padding) {
		buf[i++] = '0';
	}
	*padding = 0;

	if (is_unsigned == false && (xs < 0))
		buf[i++] = '-';

	if (alt_form && base == 16) {
		putfunc('0', static_foreg, static_backg);
		putfunc('x', static_foreg, static_backg);
		alt_form = 0;
	}
	while (--i >= 0)
		putfunc(buf[i], static_foreg, static_backg);
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
	if (color <= 29)
		return;
	if (fg)
		set_term_color(color, 0, true, false);
	else
		set_term_color(0, color, false, true);
}
__attribute__((format(printf, 1, 2)))
__nonnull(1) void uart_cprintf(const char *fmt, ...)
{
	cons.locking = 0;
	va_list argp;
	va_start(argp, fmt);
	vcprintf(uartputc3, fmt, argp);
	va_end(argp);
	cons.locking = 1;
}

__attribute__((format(printf, 1, 2))) __nonnull(1) void cprintf(const char *fmt,
																																...)
{
	va_list argp;
	va_start(argp, fmt);
	vcprintf(vga_write_char, fmt, argp);
	va_end(argp);
}
__attribute__((format(printf, 1, 2)))
__nonnull(1) void vga_cprintf(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vcprintf(vga_write_char, fmt, argp);
	va_end(argp);
}
// Print to the console. only understands %d, %x, %p, %s.
__nonnull(1) static void vcprintf(putfunc_t putfunc, const char *fmt,
																	va_list argp)
{
	int c, locking;
	size_t i;
	char *s;
	int padding = 0;

	locking = cons.locking;
	if (locking)
		acquire(&cons.lock);

	for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
		if (c != '%') {
			// ANSI color escape sequences.
			// All three of these equal each other, but
			// they are there for documentation and grepping purposes.
			if (c == '\033' || c == '\x1b' || c == '\e') {
				// Rust has a crate for doing this sort of stuff.
				// It acts as a 'mini parser' that sends us various
				// types, and we have to make sense of them.
				i += let_rust_handle_it(fmt + i);
				continue;
			}
			putfunc(c, static_foreg, static_backg);
			continue;
		}
do_again:
		c = fmt[++i] & 0xff;
		if (c == 0)
			break;
		switch (c) {
		case 'u':
			if (long_form == 0) {
				unsigned int ud = va_arg(argp, unsigned int);
				printint(putfunc, ud, ud, 10, 0, &padding);
			} else {
				unsigned long ul = va_arg(argp, unsigned long);
				printint(putfunc, ul, ul, 10, 0, &padding);
			}
			break;
		case 'd':
			if (long_form == 0) {
				int d = va_arg(argp, int);
				printint(putfunc, d, d, 10, 1, &padding);
			} else {
				long ld = va_arg(argp, long);
				printint(putfunc, ld, ld, 10, 1, &padding);
			}
			break;
		case '0':
			zero_form = 1;
			padding = 18; // TODO make dynamic
			goto do_again;
		case '#':
			alt_form = 1;
			goto do_again;
		case 'l':
			long_form = 1;
			goto do_again;
		case 'x':
			if (long_form == 0) {
				unsigned int x = va_arg(argp, unsigned int);
				printint(putfunc, x, x, 16, 1, &padding);
			} else {
				unsigned long lx = va_arg(argp, unsigned long);
				printint(putfunc, lx, lx, 16, 1, &padding);
			}
			break;
		case 'p':
			alt_form = 1;
			uintptr_t p = (uintptr_t)va_arg(argp, void *);
			printint(putfunc, p, p, 16, 1, &padding);
			break;
		case 'o':
			if (long_form == 0) {
				unsigned int o = va_arg(argp, unsigned int);
				printint(putfunc, o, o, 8, 0, &padding);
			} else {
				unsigned int lo = va_arg(argp, unsigned int);
				printint(putfunc, lo, lo, 8, 0, &padding);
			}
			break;
		case 's':
			if ((s = va_arg(argp, char *)) == 0)
				s = "(null)";
			for (; *s; s++)
				putfunc(*s, static_foreg, static_backg);
			break;
		case '%':
			putfunc('%', static_foreg, static_backg);
			break;
		default:
			// Print unknown % sequence to draw attention.
			putfunc('%', static_foreg, static_backg);
			putfunc(c, static_foreg, static_backg);
			break;
		}
skip_printing:;
	}

	if (locking)
		release(&cons.lock);
}

__noreturn __cold void
panic(const char *s)
{
	int i;
	uintptr_t pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	uart_cprintf("lapicid %d: panic: ", lapicid());
	uart_cprintf("%s", s);
	uart_cprintf("\n");
	getcallerpcs(&s, pcs);
	for (i = 0; i < 10; i++)
		uart_cprintf(" %#lx", pcs[i]);
	panicked = 1; // freeze other CPU
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
	if (panicked) {
		cli();
		for (;;)
			;
	}

	if (c == BACKSPACE) {
		uartputc('\b');
		uartputc(' ');
		uartputc('\b');
	} else
		uartputc(c);
	vga_write(c, static_foreg, static_backg);
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint32_t r; // Read index
	uint32_t w; // Write index
	uint32_t e; // Edit index
} input;

#define C(x) ((x) - '@') // Control-x

void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;

	acquire(&cons.lock);
	while ((c = getc()) >= 0) {
		switch (c) {
		case C('P'): // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			doprocdump = 1;
			break;
		case C('U'): // Kill line.
			while (input.e != input.w &&
						 input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
				input.e--;
				vga_write_char(BACKSPACE, static_foreg, static_backg);
			}
			break;
		case C('H'):
		case '\x7f': // Backspace
			if (input.e != input.w) {
				input.e--;
				vga_write_char(BACKSPACE, static_foreg, static_backg);
			}
			break;
		default:
			if (c != 0 && input.e - input.r < INPUT_BUF) {
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				if (c != C('D'))
					vga_write_char(c, static_foreg, static_backg);
				if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
					if (c == C('D'))
						vga_write_char('\n', static_foreg, static_backg);
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
}
__nonnull(1, 2) static int consoleread(struct inode *ip, char *dst, int n)
{
	uint32_t target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while (n > 0) {
		while (input.r == input.w) {
			if (myproc()->killed) {
				release(&cons.lock);
				ilock(ip);
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
		if (c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);

	return target - n;
}
/*
 * INVARIANT: None of the console_{height,width}_{text,pixels} functions should be
 * called before parse_multiboot(struct multiboot_info *).
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

int
console_width_text(void)
{
	return __multiboot_console_width_text();
}

int
console_height_text(void)
{
	return __multiboot_console_height_text();
}
/* clang-format off */
// This is asynchronous.
__nonnull(1, 2) static int
consolewrite(__attribute__((unused)) struct inode *ip,
																		 char *buf, int n)
{
	for (int i = 0; i < n; i++) {
		vga_write_char(buf[i] & 0xff, static_foreg, static_backg);
	}

	return n;
}
/* clang-format on */

void
consoleinit(void)
{
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}
