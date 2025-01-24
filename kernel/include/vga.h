#pragma once
#define WIDTH 640
#define HEIGHT 480
#define BPP_DEPTH 32
#ifndef __ASSEMBLER__
#include "../boot/multiboot2.h"

#define R(x) (x << 16)
#define G(x) (x << 8)
#define B(x) (x << 0)
enum {
	VGA_COLOR_BLACK = 0x000000,
	VGA_COLOR_RED = R(170) | G(0) | B(0),
	VGA_COLOR_GREEN = R(0) | G(170) | B(0),
	VGA_COLOR_YELLOW = R(170) | G(85) | B(0),
	VGA_COLOR_BLUE = R(0) | G(0) | B(170),
	VGA_COLOR_MAGENTA = R(170) | G(0) | B(170),
	VGA_COLOR_CYAN = R(0) | G(170) | B(170),
	VGA_COLOR_WHITE = R(170) | G(170) | B(170),
	VGA_COLOR_BRIGHT_BLACK = R(85) | B(85) | G(85),
	VGA_COLOR_BRIGHT_RED = R(255) | G(85) | B(85),
	VGA_COLOR_BRIGHT_GREEN = R(85) | G(255) | B(85),
	VGA_COLOR_BRIGHT_YELLOW = R(255) | G(255) | B(85),
	VGA_COLOR_BRIGHT_BLUE = R(85) | G(85) | B(255),
	VGA_COLOR_BRIGHT_MAGENTA = R(255) | G(85) | B(255),
	VGA_COLOR_BRIGHT_CYAN = R(85) | G(255) | B(255),
	VGA_COLOR_BRIGHT_WHITE = R(255) | G(255) | B(255),
};
struct vga_rectangle {
	uint32_t x;
	uint32_t y;
	uint32_t xlen;
	uint32_t ylen;
};
void
vga_init(struct multiboot_tag_framebuffer *tag, struct fb_rgb rgb,
				 struct multiboot_tag_framebuffer_common common);
void
vga_write_char(int c, uint32_t foreground, uint32_t background);
// Raw pixel writing.
void
vga_write(uint32_t x, uint32_t y, uint32_t color);
void
vga_fill_rect(struct vga_rectangle rect, uint32_t hex_color);
#endif
