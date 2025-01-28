#include "stdbool.h"
#include "vga.h"
#include "x86.h"
#include "traps.h"
#include "ioapic.h"
#include "macros.h"
#include <stdint.h>
#define MOUSE_STATUS 0x64
#define MOUSE_DATA 0x60
#define ENABLE_AUX_DEVICE 0xA8
#define MOUSE_CONTROLLER_RAM_WRITE 0x60
#define MOUSE_CONTROLLER_RAM_READ 0x20
#define WRITE_TO_AUX 0xD4
#define MOUSEID 0xF2

#define BUTTON_LEFT 1 << 0
#define BUTTON_RIGHT 1 << 1
#define BUTTON_MIDDLE 1 << 2
#define MOUSE_XSIGN 1 << 4
#define MOUSE_YSIGN 1 << 5
#define MOUSE_ALWAYS_SET 0xC0

static uint8_t mouse_data[4];
static uint8_t count = 0;

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
	ioapicenable(IRQ_PS2_MOUSE, 0);
}

static bool left = 0;
static bool right = 0;
static bool middle = 0;

static uint32_t x = 0;
static uint32_t y = 0;

void
ps2mouseintr(void)
{
	uint8_t status;
	mouse_recv();
	left = false;
	right = false;
	middle = false;

	status = inb(MOUSE_STATUS) & 1;
	if (status) {
		mouse_recv();
		mouse_data[count++] = inb(MOUSE_DATA);
		if (count == 3) {
			count = 0;
			int32_t delta_x = mouse_data[1] | ((mouse_data[0] & (1 << 4)) ? 0xFFFFFF00 : 0);
			int32_t delta_y = mouse_data[2] | ((mouse_data[0] & (1 << 5)) ? 0xFFFFFF00 : 0);

			// Mouse x/y overflow (bits 6 and 7)
			if ((mouse_data[0] & 0x80) != 0 || (mouse_data[0] & 0x40) != 0) {
				delta_x = 0;
				delta_y = 0;
				return;
			}

			if ((mouse_data[0] & BUTTON_LEFT)) {
				left = true;
			}
			if ((mouse_data[0] & BUTTON_RIGHT)) {
				right = true;
			}
			if ((mouse_data[0] & BUTTON_MIDDLE)) {
				middle = true;
			}
			x = signed_saturating_add(x, delta_x, WIDTH-1);

			y = signed_saturating_add(y, -delta_y, HEIGHT-1);

			vga_write(x, y, 0xff00ff);
		}
	}
}
