#include "vga.h"
#include "boot/multiboot2.h"
#include "memlayout.h"
#include "macros.h"
#include "font.h"
#include "kernel_assert.h"
#include "console.h"
#include "kalloc.h"
#include "uart.h"
#include "kernel_string.h"
#include <stddef.h>
#include <stdint.h>

#define TAB_WIDTH 4
static struct multiboot_tag_framebuffer *fb_data = NULL;
static struct fb_rgb fb_rgb = {0};
static struct multiboot_tag_framebuffer_common fb_common = {0};
const uint32_t COLOR_RED;
#define INTERNAL_COLOR_RED (((1 << fb_rgb.framebuffer_red_mask_size) - 1) << fb_rgb.framebuffer_red_field_position)
#define INTERNAL_COLOR_GREEN (((1 << fb_rgb.framebuffer_green_mask_size) - 1) << fb_rgb.framebuffer_green_field_position)
#define INTERNAL_COLOR_BLUE (((1 << fb_rgb.framebuffer_blue_mask_size) - 1) << fb_rgb.framebuffer_blue_field_position)
#define INTERNAL_COLOR_WHITE INTERNAL_COLOR_BLUE | INTERNAL_COLOR_GREEN | INTERNAL_COLOR_RED
#define INTERNAL_COLOR_BLACK 0

/*
 * This init function needs these 3 parameters
 * because at some point during kernel bring-up,
 * the pointers get invalidated. We store them using
 * copies here.
 */
void
vga_init(struct multiboot_tag_framebuffer *tag, struct fb_rgb rgb, struct multiboot_tag_framebuffer_common common)
{
	fb_data = tag;
	fb_rgb = rgb;
	fb_common = common;
}

// The color is in hex: 0xRRGGBB
void
vga_write(uint32_t x, uint32_t y, uint32_t color)
{
	kernel_assert(fb_data != NULL);

	void *fb = IO2V(fb_common.framebuffer_addr);
	multiboot_uint32_t *pixel = fb + fb_common.framebuffer_pitch * y + (fb_common.framebuffer_bpp / 8) * x;
	*pixel = color;
}

void
vga_fill_rect(struct vga_rectangle rect, uint32_t hex_color)
{
	for (uint32_t x = 0; x < rect.xlen; x++) {
		for (uint32_t y = 0; y < rect.ylen; y++) {
			vga_write(x+rect.x, y+rect.y, hex_color);
		}
	}

}

static void
render_font_glyph(uint8_t character, uint32_t x, uint32_t y, uint8_t width, uint8_t height, const uint8_t font[static 256][width * height],
									uint32_t foreground, uint32_t background)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			uint32_t adjusted_x = x + j;
			uint32_t adjusted_y = y + i;
			if (font[character][i*width + j] == 1)
				vga_write(adjusted_x, adjusted_y, foreground);
			else
				vga_write(adjusted_x, adjusted_y, background);

		}
	}
}
static uint32_t width_chars(uint8_t width) {
	return WIDTH / width;
}
static uint32_t height_chars(uint8_t height) {
	return HEIGHT / height;
}
static uint32_t fb_char_index = 0;
#define ROUND_UP(x, round_to) (((x + (round_to-1) )/ round_to ) * round_to)

static uint32_t
pixel_count_to_x_coord(uint8_t font_width)
{
	return (fb_char_index / font_width) % width_chars(font_width);
}

static uint32_t
pixel_count_to_y_coord(uint8_t font_width)
{
	// pixels to chars here
	return (fb_char_index / font_width) / width_chars(font_width);
}

static void
vga_write_newline(uint32_t fb_width, uint8_t font_width, uint32_t height)
{
	fb_char_index = ROUND_UP(fb_char_index, fb_width);
}
static void
vga_write_tab(uint32_t fb_width, uint8_t font_width, uint32_t height)
{
	fb_char_index += TAB_WIDTH * font_width;
}
static void
vga_write_carriage_return(uint32_t width, uint32_t height)
{
	fb_char_index += width;
}

static void
vga_backspace(uint8_t font_width)
{
	fb_char_index -= font_width;
	vga_write_char(' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
	fb_char_index -= font_width;
}

static void
vga_scroll(uint32_t fb_width, uint32_t fb_height, uint8_t font_width, uint8_t font_height)
{
	// Inplace scrolling.
	uint32_t *fb_memory = IO2V(fb_common.framebuffer_addr);

	uint32_t fb_in_bytes = fb_height * fb_width;

	// Initiate scroll.
	for (int i = fb_width * font_height; i < fb_in_bytes; i++) {
		fb_memory[i - fb_width * font_height] = fb_memory[i];
	}
	// Clear the last row.
	for (int i = ((fb_in_bytes-1) - fb_width * font_height); i < fb_in_bytes; i++) {
		fb_memory[i] = 0;
	}

	// We need to:
	// 1. round up to the nearest fb_width
	// 2. subtract the width.
	fb_char_index = ROUND_UP(fb_char_index, fb_width) - fb_width;

}

// Consistent with the define found in console.c.
#define BACKSPACE 0x100

void
vga_write_char(int c, uint32_t foreground, uint32_t background) {
	struct font_data_8x16 data = {8, 16, &font_default };
	if (fb_char_index >= WIDTH * height_chars(data.height) ||
		((fb_char_index >= (WIDTH * (height_chars(data.height)-1))) && (c == '\n' || c == '\r'))) {
			vga_scroll(WIDTH, HEIGHT, data.width, data.height);
	} else if (c == '\n') {
		vga_write_newline(WIDTH, data.width, HEIGHT);
	} else if (c == '\t') {
		vga_write_tab(WIDTH, data.width, HEIGHT);
	} else if (c == '\b' || c == BACKSPACE) {
		vga_backspace(data.width);
	} else {
		render_font_glyph(c, pixel_count_to_x_coord(data.width) * data.width,
										pixel_count_to_y_coord(data.width) * data.height,
										data.width, data.height, *data.font,
										foreground, background);
		fb_char_index += data.width;
	}
	if (c == BACKSPACE) {
		uartputc('\b');
		uartputc(' ');
		uartputc('\b');
	} else {
		uartputc(c);
	}
}
