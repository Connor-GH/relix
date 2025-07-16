#include "ps2mouse.h"
#include "console.h"
#include "errno.h"
#include "ioapic.h"
#include "kalloc.h"
#include "lib/queue.h"
#include "proc.h"
#include "spinlock.h"
#include "traps.h"
#include "x86.h"
#include <bits/fcntl_constants.h>

#include "mman.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define MOUSE_STATUS 0x64
#define MOUSE_DATA 0x60
#define ENABLE_AUX_DEVICE 0xA8
#define MOUSE_CONTROLLER_RAM_WRITE 0x60
#define MOUSE_CONTROLLER_RAM_READ 0x20
#define WRITE_TO_AUX 0xD4
#define MOUSEID 0xF2

#define MOUSE_QUEUE_SIZE 256

static uint8_t mouse_data[3];
static uint8_t count = 0;

struct {
	struct spinlock lock;
	struct queue_mouse_packet *mouse_queue;
} mouselock;

static void
mouse_wait(uint8_t a_type)
{
	uint32_t time_out = 1000000;
	if (a_type == 0) {
		while (time_out-- > 0) {
			if ((inb(0x64) & 1) == 1) {
				return;
			}
		}
	} else {
		while (time_out-- > 0) {
			if ((inb(0x64) & 2) == 0) {
				return;
			}
		}
	}
}

static void
mouse_recv(void)
{
	mouse_wait(1);
}

static void
mouse_send(void)
{
	mouse_wait(0);
}

void
mouse_command(uint8_t command)
{
	uint8_t ack;
	mouse_send();
	outb(MOUSE_STATUS, WRITE_TO_AUX);
	mouse_send();
	outb(MOUSE_DATA, command);
	mouse_recv();

	do {
		ack = inb(MOUSE_DATA);
		// Acknowledge byte
	} while (ack != 0xFA);
}

static void
mouse_write(uint8_t byte)
{
	mouse_wait(1);
	outb(MOUSE_STATUS, WRITE_TO_AUX);
	mouse_wait(1);
	outb(MOUSE_DATA, byte);
}

static uint8_t
mouse_read(void)
{
	mouse_wait(0);
	return inb(MOUSE_DATA);
}

__nonnull(2, 3) static ssize_t
	mouseread(__unused short minor, struct inode *ip, char *dst, size_t n)
{
get_element:;
	acquire(&mouselock.lock);
	struct mouse_packet data;
	int ret = dequeue_mouse_packet(mouselock.mouse_queue, &data, kfree);
	release(&mouselock.lock);
	if (ret != QUEUE_SUCCESS) {
		if ((ip->flags & O_NONBLOCK) == O_NONBLOCK) {
			return -EWOULDBLOCK;
		} else {
			// Let other processes do stuff while we sit here and spin.
			yield();
			// The default is spinning on the mouse queue.
			goto get_element;
		}
	}
	memcpy(dst, &data, sizeof(data));
	return n;
}

/* clang-format off */
__nonnull(2, 3) static ssize_t
mousewrite(__unused short minor, __unused struct inode *ip,
					 __unused char *buf, size_t n)
{
	return n;
}
/* clang-format on */

static struct mmap_info
mousemmap_noop(__unused short minor, __unused size_t length,
               __unused uintptr_t addr, __unused int perm)
{
	return (struct mmap_info){};
}

static int mouse_file_ref = 0;

static int
mouseopen(__unused short minor, __unused int flags)
{
	if (mouse_file_ref == 0) {
		acquire(&mouselock.lock);
		clean_queue_mouse_packet(mouselock.mouse_queue, kfree);
		release(&mouselock.lock);
		mouse_file_ref++;
	}
	return 0;
}

static int
mouseclose(__unused short minor)
{
	mouse_file_ref--;
	return 0;
}

void
ps2mouseinit(void)
{
	uint8_t status;
	mouse_wait(1);
	outb(MOUSE_STATUS, MOUSE_CONTROLLER_RAM_READ);
	mouse_wait(0);
	// set bit 1, clear bit 5
	status = (inb(MOUSE_DATA) | 2) & ~0x20;
	mouse_wait(1);
	outb(MOUSE_STATUS, MOUSE_CONTROLLER_RAM_WRITE);
	mouse_wait(1);
	outb(MOUSE_DATA, status);

	// Defaults
	mouse_write(0xF6);
	mouse_read(); // ACK

	// Enable
	mouse_write(0xF4);
	mouse_read(); // ACK

	initlock(&mouselock.lock, "mouse");

	acquire(&mouselock.lock);
	mouselock.mouse_queue = create_queue_mouse_packet(kmalloc);
	release(&mouselock.lock);

	if (mouselock.mouse_queue == NULL) {
		panic("Could not create mouse_queue");
	}

	devsw[MOUSE].write = mousewrite;
	devsw[MOUSE].read = mouseread;
	devsw[MOUSE].mmap = mousemmap_noop;
	devsw[MOUSE].open = mouseopen;
	devsw[MOUSE].close = mouseclose;
	ioapicenable(IRQ_PS2_MOUSE, 0);
}

void
ps2mouseintr(void)
{
	uint8_t status;
	mouse_recv();

	status = inb(MOUSE_STATUS) & 1;
	if (status) {
		mouse_recv();
		mouse_data[count++] = inb(MOUSE_DATA);
		if (count == 3) {
			count = 0;

			struct mouse_packet coords = { .data = { mouse_data[0], mouse_data[1],
				                                       mouse_data[2] } };
			int val;
			acquire(&mouselock.lock);
			val = enqueue_mouse_packet(mouselock.mouse_queue, coords, kmalloc,
			                           MOUSE_QUEUE_SIZE);
			release(&mouselock.lock);

			if (val == QUEUE_OOM) {
				panic("Cannot allocate memory for mouse queue");
			}
		}
	}
}
