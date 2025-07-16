#include "kbd.h"
#include "console.h"
#include "errno.h"
#include "fs.h"
#include "ioapic.h"
#include "kalloc.h"
#include "lib/queue.h"
#include "mman.h"
#include "proc.h"
#include "spinlock.h"
#include "string.h"
#include "traps.h"
#include "x86.h"
#include <bits/fcntl_constants.h>
#include <stdint.h>

// PC keyboard interface constants

#define KBSTATP 0x64 // kbd controller status port(I)
#define KBS_DIB 0x01 // kbd data in buffer
#define KBDATAP 0x60 // kbd data port(I)

#define NO 0

#define SHIFT (1 << 0)
#define CTL (1 << 1)
#define ALT (1 << 2)

#define CAPSLOCK (1 << 3)
#define NUMLOCK (1 << 4)
#define SCROLLLOCK (1 << 5)

#define E0ESC (1 << 6)

// Special keycodes
#define KEY_HOME 0xE0
#define KEY_END 0xE1
#define KEY_UP 0xE2
#define KEY_DN 0xE3
#define KEY_LF 0xE4
#define KEY_RT 0xE5
#define KEY_PGUP 0xE6
#define KEY_PGDN 0xE7
#define KEY_INS 0xE8
#define KEY_DEL 0xE9

// C('A') == Control-A
#define C(x) (x - '@')

const uint8_t shiftcode[256] = { [0x1D] = CTL, [0x2A] = SHIFT, [0x36] = SHIFT,
	                               [0x38] = ALT, [0x9D] = CTL,   [0xB8] = ALT };

const uint8_t
	togglecode[256] = { [0x3A] = CAPSLOCK, [0x45] = NUMLOCK, [0x46] = SCROLLLOCK };

const uint8_t normalmap[256] = { NO,
	                               0x1B,
	                               '1',
	                               '2',
	                               '3',
	                               '4',
	                               '5',
	                               '6', // 0x00
	                               '7',
	                               '8',
	                               '9',
	                               '0',
	                               '-',
	                               '=',
	                               '\b',
	                               '\t',
	                               'q',
	                               'w',
	                               'e',
	                               'r',
	                               't',
	                               'y',
	                               'u',
	                               'i', // 0x10
	                               'o',
	                               'p',
	                               '[',
	                               ']',
	                               '\n',
	                               NO,
	                               'a',
	                               's',
	                               'd',
	                               'f',
	                               'g',
	                               'h',
	                               'j',
	                               'k',
	                               'l',
	                               ';', // 0x20
	                               '\'',
	                               '`',
	                               NO,
	                               '\\',
	                               'z',
	                               'x',
	                               'c',
	                               'v',
	                               'b',
	                               'n',
	                               'm',
	                               ',',
	                               '.',
	                               '/',
	                               NO,
	                               '*', // 0x30
	                               NO,
	                               ' ',
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               NO,
	                               '7', // 0x40
	                               '8',
	                               '9',
	                               '-',
	                               '4',
	                               '5',
	                               '6',
	                               '+',
	                               '1',
	                               '2',
	                               '3',
	                               '0',
	                               '.',
	                               NO,
	                               NO,
	                               NO,
	                               NO, // 0x50
	                               [0x9C] = '\n', // KP_Enter
	                               [0xB5] = '/', // KP_Div
	                               [0xC8] = KEY_UP,
	                               [0xD0] = KEY_DN,
	                               [0xC9] = KEY_PGUP,
	                               [0xD1] = KEY_PGDN,
	                               [0xCB] = KEY_LF,
	                               [0xCD] = KEY_RT,
	                               [0x97] = KEY_HOME,
	                               [0xCF] = KEY_END,
	                               [0xD2] = KEY_INS,
	                               [0xD3] = KEY_DEL };

const uint8_t shiftmap[256] = { NO,
	                              033,
	                              '!',
	                              '@',
	                              '#',
	                              '$',
	                              '%',
	                              '^', // 0x00
	                              '&',
	                              '*',
	                              '(',
	                              ')',
	                              '_',
	                              '+',
	                              '\b',
	                              '\t',
	                              'Q',
	                              'W',
	                              'E',
	                              'R',
	                              'T',
	                              'Y',
	                              'U',
	                              'I', // 0x10
	                              'O',
	                              'P',
	                              '{',
	                              '}',
	                              '\n',
	                              NO,
	                              'A',
	                              'S',
	                              'D',
	                              'F',
	                              'G',
	                              'H',
	                              'J',
	                              'K',
	                              'L',
	                              ':', // 0x20
	                              '"',
	                              '~',
	                              NO,
	                              '|',
	                              'Z',
	                              'X',
	                              'C',
	                              'V',
	                              'B',
	                              'N',
	                              'M',
	                              '<',
	                              '>',
	                              '?',
	                              NO,
	                              '*', // 0x30
	                              NO,
	                              ' ',
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              NO,
	                              '7', // 0x40
	                              '8',
	                              '9',
	                              '-',
	                              '4',
	                              '5',
	                              '6',
	                              '+',
	                              '1',
	                              '2',
	                              '3',
	                              '0',
	                              '.',
	                              NO,
	                              NO,
	                              NO,
	                              NO, // 0x50
	                              [0x9C] = '\n', // KP_Enter
	                              [0xB5] = '/', // KP_Div
	                              [0xC8] = KEY_UP,
	                              [0xD0] = KEY_DN,
	                              [0xC9] = KEY_PGUP,
	                              [0xD1] = KEY_PGDN,
	                              [0xCB] = KEY_LF,
	                              [0xCD] = KEY_RT,
	                              [0x97] = KEY_HOME,
	                              [0xCF] = KEY_END,
	                              [0xD2] = KEY_INS,
	                              [0xD3] = KEY_DEL };

const uint8_t ctlmap[256] = { NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            C('Q'),
	                            C('W'),
	                            C('E'),
	                            C('R'),
	                            C('T'),
	                            C('Y'),
	                            C('U'),
	                            C('I'),
	                            C('O'),
	                            C('P'),
	                            NO,
	                            NO,
	                            '\r',
	                            NO,
	                            C('A'),
	                            C('S'),
	                            C('D'),
	                            C('F'),
	                            C('G'),
	                            C('H'),
	                            C('J'),
	                            C('K'),
	                            C('L'),
	                            NO,
	                            NO,
	                            NO,
	                            NO,
	                            C('\\'),
	                            C('Z'),
	                            C('X'),
	                            C('C'),
	                            C('V'),
	                            C('B'),
	                            C('N'),
	                            C('M'),
	                            NO,
	                            NO,
	                            C('/'),
	                            NO,
	                            NO,
	                            [0x9C] = '\r', // KP_Enter
	                            [0xB5] = C('/'), // KP_Div
	                            [0xC8] = KEY_UP,
	                            [0xD0] = KEY_DN,
	                            [0xC9] = KEY_PGUP,
	                            [0xD1] = KEY_PGDN,
	                            [0xCB] = KEY_LF,
	                            [0xCD] = KEY_RT,
	                            [0x97] = KEY_HOME,
	                            [0xCF] = KEY_END,
	                            [0xD2] = KEY_INS,
	                            [0xD3] = KEY_DEL };
static int
kbdgetc_raw(void)
{
	uint32_t st, data;

	st = inb(KBSTATP);
	if ((st & KBS_DIB) == 0) {
		return -1;
	}
	data = inb(KBDATAP);
	return data;
}

int
kbd_scancode_into_char(uint32_t data)
{
	static uint32_t shift;
	const uint8_t *charcode[4] = { normalmap, shiftmap, ctlmap, ctlmap };
	uint32_t st, c;

	if (data == 0xE0) {
		shift |= E0ESC;
		return 0;
	} else if (data & 0x80) {
		// Key released
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	} else if (shift & E0ESC) {
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data];
	shift ^= togglecode[data];
	c = charcode[shift & (CTL | SHIFT)][data];
	if (shift & CAPSLOCK) {
		if ('a' <= c && c <= 'z') {
			c += 'A' - 'a';
		} else if ('A' <= c && c <= 'Z') {
			c += 'a' - 'A';
		}
	}
	return c;
}

void
display_queue(struct queue_unsigned_char *q)
{
	if (q == NULL) {
		return;
	}
	uart_printf("elements: ");
	unsigned char data;
	int ret = dequeue_unsigned_char(q, &data, kfree);
	while (ret != -1) {
		uart_printf("%d (%c)", data, data);
		data = dequeue_unsigned_char(q, &data, kfree);
	}
	uart_printf("\n");
}

#define KEYBOARD_QUEUE_SIZE 16

void
kbdintr(void)
{
	consoleintr(kbdgetc_raw);
}

struct {
	struct spinlock lock;
	struct queue_unsigned_char *kbd_queue;
} kbdlock;

int
kbd_enqueue(unsigned char value)
{
	int val;
	acquire(&kbdlock.lock);
	val = enqueue_unsigned_char(kbdlock.kbd_queue, value, kmalloc,
	                            KEYBOARD_QUEUE_SIZE);
	release(&kbdlock.lock);
	return val;
}

__nonnull(2, 3) static ssize_t
	kbdread(short minor, struct inode *ip, char *dst, size_t n)
{
get_element:;
	acquire(&kbdlock.lock);
	unsigned char data;
	int ret = dequeue_unsigned_char(kbdlock.kbd_queue, &data, kfree);
	release(&kbdlock.lock);
	if (ret != QUEUE_SUCCESS) {
		if ((ip->flags & O_NONBLOCK) == O_NONBLOCK) {
			return -EWOULDBLOCK;
		} else {
			// Let other processes do stuff while we sit here and spin.
			yield();
			// The default is spinning on the kbd queue.
			goto get_element;
		}
	}
	memcpy(dst, &data, sizeof(data));
	return n;
}

/* clang-format off */
__nonnull(2, 3) static ssize_t
kbdwrite(short minor, __attribute__((unused)) struct inode *ip,
																		 char *buf, size_t n)
{
	return n;
}
/* clang-format on */

static struct mmap_info
kbdmmap_noop(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){};
}

static int kbd_file_ref = 0;

static int
kbdopen(short minor, int flags)
{
	if (kbd_file_ref == 0) {
		acquire(&kbdlock.lock);
		clean_queue_unsigned_char(kbdlock.kbd_queue, kfree);
		release(&kbdlock.lock);
		kbd_file_ref++;
	}
	return 0;
}

static int
kbdclose(short minor)
{
	kbd_file_ref--;
	return 0;
}

void
kbdinit(void)
{
	initlock(&kbdlock.lock, "kbd");

	acquire(&kbdlock.lock);
	kbdlock.kbd_queue = create_queue_unsigned_char(kmalloc);
	release(&kbdlock.lock);

	if (kbdlock.kbd_queue == NULL) {
		panic("Could not create kbd_queue");
	}

	devsw[KBD].write = kbdwrite;
	devsw[KBD].read = kbdread;
	devsw[KBD].mmap = kbdmmap_noop;
	devsw[KBD].open = kbdopen;
	devsw[KBD].close = kbdclose;

	ioapicenable(IRQ_KBD, 0);
}
