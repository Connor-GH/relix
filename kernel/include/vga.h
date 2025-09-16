#ifndef _VGA_H
#define _VGA_H
#pragma once
// ATTENTION!!!
// Might need to adjust memory in kernel/main.c
// if this changes.
#define SCREEN_WIDTH 640U
#define SCREEN_HEIGHT 480U
#define SCREEN_BPP_DEPTH 32U
#ifndef __ASSEMBLER__
#include "boot/multiboot2.h"
#include <stdbool.h>

#define R(x) (x << 16U)
#define G(x) (x << 8U)
#define B(x) (x << 0U)
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
#if __RELIX_KERNEL__
struct vga_rectangle {
	uint32_t x;
	uint32_t y;
	uint32_t xlen;
	uint32_t ylen;
};
void vga_init(struct multiboot_tag_framebuffer *tag);
void vga_write_char(int c, uint32_t foreground, uint32_t background);
void vga_reset_char_index(void);
// Raw pixel writing.
void vga_write_pixel(uint32_t x, uint32_t y, uint32_t color);
void vga_fill_rect(struct vga_rectangle rect, uint32_t hex_color);
void clear_cells(uint32_t x, uint32_t y, uint32_t x_len, uint32_t y_len,
                 uint8_t font_height, uint8_t font_width, uint32_t foreground,
                 uint32_t background, const uint8_t (*font)[]);
void ansi_set_cursor_location_x(uint16_t x);
void ansi_set_cursor_location_y(uint16_t y);
void ansi_set_cursor_location(uint16_t x, uint16_t y);
#endif
#endif
#endif // !_VGA_H
