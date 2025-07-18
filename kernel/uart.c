// Intel 8250 serial port (UART).

#include "uart.h"
#include "console.h"
#include "drivers/lapic.h"
#include "ioapic.h"
#include "traps.h"
#include "x86.h"

#define COM1 0x3f8

static int uart; // is there a uart?

void
uartinit1(void)
{
	char *p;

	// Turn off the FIFO
	outb(COM1 + 2, 0);

	// 9600 baud, 8 data bits, 1 stop bit, parity off.
	outb(COM1 + 3, 0x80); // Unlock divisor
	outb(COM1 + 0, 115200 / 9600);
	outb(COM1 + 1, 0);
	outb(COM1 + 3, 0x03); // Lock divisor, 8 data bits.
	outb(COM1 + 4, 0);
	outb(COM1 + 1, 0x01); // Enable receive interrupts.

	// If status is 0xFF, no serial port.
	if (inb(COM1 + 5) == 0xFF) {
		return;
	}
	uart = 1;

	// Announce that we're here.
	for (p = "booting relix...\n"; *p; p++) {
		uartputc(*p);
	}
}

void
uartinit2(void)
{
	// Acknowledge pre-existing interrupt conditions;
	// enable interrupts.
	inb(COM1 + 2);
	inb(COM1 + 0);
	ioapicenable(IRQ_COM1, 0);
}

void
uartputc(int c)
{
	if (!uart) {
		return;
	}
	for (int i = 0; i < 128 && !(inb(COM1 + 5) & 0x20); i++) {
		microdelay(10);
	}
	outb(COM1 + 0, c);
}

static int
uartgetc(void)
{
	if (!uart) {
		return -1;
	}
	if (!(inb(COM1 + 5) & 0x01)) {
		return -1;
	}
	return inb(COM1 + 0);
}

void
uartintr(void)
{
	consoleintr(uartgetc);
}
