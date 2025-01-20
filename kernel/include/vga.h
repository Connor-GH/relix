#pragma once
#define WIDTH 640
#define HEIGHT 480
#define BPP_DEPTH 32
#ifndef __ASSEMBLER__
#include "../boot/multiboot2.h"

enum vga_color {
	VGA_COLOR_RED,
	VGA_COLOR_GREEN,
	VGA_COLOR_BLUE,
	VGA_COLOR_PURPLE,
	VGA_COLOR_WHITE,
	VGA_COLOR_BLACK,
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
#endif
