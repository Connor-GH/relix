#pragma once
#define WIDTH 640
#define HEIGHT 480
#define BPP_DEPTH 32
#ifndef __ASSEMBLER__
#include "../boot/multiboot2.h"

enum {
	VGA_COLOR_BLACK = 0x000000,
	VGA_COLOR_WHITE = 0xffffff,
	VGA_COLOR_PURPLE = 0xff00ff,
	VGA_COLOR_RED = 0xff0000,
	VGA_COLOR_BLUE = 0x0000ff,
	VGA_COLOR_GREEN = 0x00ff00,
};
struct vga_rectangle {
	uint32_t x;
	uint32_t y;
	uint32_t xlen;
	uint32_t ylen;
};
void
vga_init(struct multiboot_tag_framebuffer *tag,
		struct fb_rgb rgb, struct multiboot_tag_framebuffer_common common);
void
vga_write_char(int c, uint32_t foreground, uint32_t background);
// Raw pixel writing.
void
vga_write(uint32_t x, uint32_t y, uint32_t color);
void
vga_fill_rect(struct vga_rectangle rect, uint32_t hex_color);
#endif
