#include "vga.h"
#include "boot/multiboot2.h"
#include "file.h"
#include "fs.h"
#include "memlayout.h"
#include "font.h"
#include "kernel_assert.h"
#include "macros.h"
#include "mman.h"
#include "uart.h"
#include "kalloc.h"
#include "console.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TAB_WIDTH 4
static struct multiboot_tag_framebuffer *fb_data = NULL;
static struct fb_rgb fb_rgb = { 0 };
static struct multiboot_tag_framebuffer_common fb_common = { 0 };
// TODO make this take a dynamic font size?
// i.e. remove the "8" and "16". We should get
// this from a font file anyways.
struct DamageTracking {
	uint32_t fg[WIDTH / 8][HEIGHT / 16];
	uint32_t bg[WIDTH / 8][HEIGHT / 16];
	char data[WIDTH / 8][HEIGHT / 16];
};

struct CursorPosition {
	uint32_t x;
	uint32_t y;
};
static struct DamageTracking damage_tracking_data = {0};
static struct CursorPosition cursor_position = {0, 0};
const uint32_t COLOR_RED;
#define INTERNAL_COLOR_RED                       \
	(((1 << fb_rgb.framebuffer_red_mask_size) - 1) \
	 << fb_rgb.framebuffer_red_field_position)
#define INTERNAL_COLOR_GREEN                       \
	(((1 << fb_rgb.framebuffer_green_mask_size) - 1) \
	 << fb_rgb.framebuffer_green_field_position)
#define INTERNAL_COLOR_BLUE                       \
	(((1 << fb_rgb.framebuffer_blue_mask_size) - 1) \
	 << fb_rgb.framebuffer_blue_field_position)
#define INTERNAL_COLOR_WHITE \
	INTERNAL_COLOR_BLUE | INTERNAL_COLOR_GREEN | INTERNAL_COLOR_RED
#define INTERNAL_COLOR_BLACK 0

static int
vgawrite(__attribute__((unused)) struct inode *ip,
				 char *buf1, int n)
{
	uint8_t *buf = (uint8_t *)buf1;
	if (buf == NULL)
		return -1;
	for (int i = 0; i < n; i+=12) {
		uint32_t x = buf[i+0] | (buf[i+1] << 8U) | (buf[i+2] << 16U) | (buf[i+3] << 24U);
		uint32_t y = buf[i+4] | (buf[i+5] << 8U) | (buf[i+6] << 16U) | (buf[i+7] << 24U);
		uint32_t color = buf[i+8] | (buf[i+9] << 8U) | (buf[i+10] << 16U) | (buf[i+11] << 24U);
		vga_write(x, y, color);
	}
	return n;
}

static int
vgaread(struct inode *ip, char *buf, int n)
{
	return 0;
}

static struct mmap_info
vgammap(size_t length, uintptr_t addr)
{
	return (struct mmap_info){WIDTH*HEIGHT*BPP_DEPTH, fb_common.framebuffer_addr, 0, NULL};
}
// INVARIANT: must be ran after vga_init().
struct multiboot_tag_framebuffer_common
get_fb_common(void)
{
	return fb_common;
}

/*
 * This init function needs these 3 parameters
 * because at some point during kernel bring-up,
 * the pointers get invalidated. We store them using
 * copies here.
 */
void
vga_init(struct multiboot_tag_framebuffer *tag, struct fb_rgb rgb,
				 struct multiboot_tag_framebuffer_common common)
{
	fb_data = tag;
	fb_rgb = rgb;
	fb_common = common;
	devsw[FB].read = vgaread;
	devsw[FB].write = vgawrite;
	devsw[FB].mmap = vgammap;
}

// The color is in hex: 0xRRGGBB
void
vga_write(uint32_t x, uint32_t y, uint32_t color)
{
	kernel_assert(fb_data != NULL);
	if (x > fb_common.framebuffer_width || y > fb_common.framebuffer_height)
		return;

	void *fb = IO2V(fb_common.framebuffer_addr);
	multiboot_uint32_t *pixel =
		fb + fb_common.framebuffer_pitch * y + (fb_common.framebuffer_bpp / 8) * x;
	*pixel = color;
}


// fb_char_index counts all pixels for width,
// but only the bottom pixels in a font for height. So for
// a 640x480 screen, that is 640 * (480/font_height).
// You index into the width by (fb_char_index % width) / font_width
// You index into the width by (fb_char_index % width)
static uint32_t fb_char_index = 0;
#define ROUND_UP(x, round_to) (((x + (round_to - 1)) / round_to) * round_to)
#define ROUND_DOWN(x, round_to) (x / round_to * round_to)

static uint32_t
pixel_count_to_char_x_coord(uint32_t char_index, uint8_t font_width)
{
	return (char_index % WIDTH) / font_width;
}

static uint32_t
pixel_count_to_char_y_coord(uint32_t char_index, uint8_t font_width)
{
	return (char_index / WIDTH);
}

static uint32_t
x_y_to_fb_char_index(uint32_t x, uint32_t y, uint8_t font_width)
{
	return (y * WIDTH) + (x * font_width);
}

static void
render_font_glyph(const uint8_t character, const uint32_t x, const uint32_t y,
									const uint8_t width,
									const uint8_t height,
									const bool font[static 256][width * height],
									const uint32_t foreground, const uint32_t background)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			const uint32_t adjusted_x = x + j;
			const uint32_t adjusted_y = y + i;
			if (font[character][i * width + j] == 1)
				vga_write(adjusted_x, adjusted_y, foreground);
			else
				vga_write(adjusted_x, adjusted_y, background);
		}
	}

}
static uint32_t
width_chars(uint8_t width)
{
	return WIDTH / width;
}
static uint32_t
height_chars(uint8_t height)
{
	return HEIGHT / height;
}


void
ansi_set_cursor_location(uint16_t x, uint16_t y)
{
	// TODO we should have some sort of font-change function.
	// Also, we should probably move some of this to userspace.
	ansi_set_cursor_location_x(x);
	ansi_set_cursor_location_y(y);
}
void
ansi_set_cursor_location_x(uint16_t x)
{
	struct font_data_8x16 font_data = { 8, 16, &font_default };
	fb_char_index = x_y_to_fb_char_index(x, cursor_position.y, font_data.width);
	cursor_position = (struct CursorPosition){x, cursor_position.y};
}

void
ansi_set_cursor_location_y(uint16_t y)
{
	struct font_data_8x16 font_data = { 8, 16, &font_default };
	fb_char_index = x_y_to_fb_char_index(cursor_position.x, y, font_data.width);
	cursor_position = (struct CursorPosition){cursor_position.x, y};
}
void
ansi_set_cursor_location_up(uint16_t by)
{
	ansi_set_cursor_location(0, saturating_sub(cursor_position.y, by, 0));
}

static void
vga_write_carriage_return(uint32_t fb_width, uint8_t font_width)
{
	fb_char_index = ROUND_UP(fb_char_index, fb_width) - fb_width;
	cursor_position = (struct CursorPosition){fb_char_index, font_width};
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
vga_backspace(uint8_t font_width, uint8_t font_height, uint32_t background)
{
	fb_char_index -= font_width;
	vga_write_char(' ', 0, background);
	fb_char_index -= font_width;
}

// This scrolling algorithm is *way* more complex than it should've been.
// The good news is that when using damage tracking for every char,
// the speedup is around 2x.
static void
vga_scroll(uint32_t fb_width, uint32_t fb_height, uint8_t font_width,
					 uint8_t font_height, const bool (*font)[])
{

	uint32_t fb_in_bytes = fb_height * fb_width;

	// Initiate scroll.
	for (int i = (fb_width * font_height); i < fb_in_bytes; i+=font_width) {
		uint32_t x = pixel_count_to_char_x_coord(i, font_width);
		uint32_t y = pixel_count_to_char_y_coord(i / font_height, font_width);
		// We "have" to scroll on these chars.
		if (!(damage_tracking_data.data[x][y] == damage_tracking_data.data[x][y-1] &&
			damage_tracking_data.fg[x][y] == damage_tracking_data.fg[x][y-1] &&
			damage_tracking_data.bg[x][y] == damage_tracking_data.bg[x][y-1])) {
			render_font_glyph(damage_tracking_data.data[x][y], x * font_width,
										 (y-1) * font_height, font_width, font_height, font,
										 damage_tracking_data.fg[x][y], damage_tracking_data.bg[x][y]);
			damage_tracking_data.data[x][y-1] = damage_tracking_data.data[x][y];
			damage_tracking_data.fg[x][y-1] = damage_tracking_data.fg[x][y];
			damage_tracking_data.bg[x][y-1] = damage_tracking_data.bg[x][y];
		}
	}
	const uint32_t height = height_chars(font_height);
	const uint32_t width = width_chars(font_width);

	// Clear the last row.
	for (int i = 0; i < width; i++) {
		damage_tracking_data.data[i][height-1] = ' ';
		damage_tracking_data.fg[i][height-1] = VGA_COLOR_WHITE;
		damage_tracking_data.bg[i][height-1] = VGA_COLOR_BLACK;

		clear_cells(i, height-1, 1, 1,
							font_width, font_height, VGA_COLOR_WHITE, VGA_COLOR_BLACK, font);
	}
	cursor_position = (struct CursorPosition){0, height-1};

	vga_write_carriage_return(fb_width, font_width);
}

void
clear_cells(uint32_t x, uint32_t y, uint32_t x_len, uint32_t y_len,
						uint8_t font_width, uint8_t font_height, uint32_t foreground,
						uint32_t background, const bool (*font)[])
{
	const uint32_t height = height_chars(font_height);
	const uint32_t width = width_chars(font_width);
	if (x > width || y > height)
		return;
	for (uint32_t i = 0; i < x_len; i++) {
		for (uint32_t j = 0; j < y_len; j++) {
			render_font_glyph(' ', (i+x)*font_width, (j+y)*font_height, font_width,
										 font_height, font, foreground, background);
		}
	}
}

void
ansi_erase_in_front_of_cursor(void)
{
	struct font_data_8x16 font_data = {8, 16, &font_default};
	uint32_t background = damage_tracking_data.bg[cursor_position.x][cursor_position.y];
	uint32_t foreground = damage_tracking_data.fg[cursor_position.x][cursor_position.y];
	/*
	 * Observe the following situation where we want to erase from '$' to 'END':
	 * $ foo bar baz
	 * more foo     END
	 *
	 * You can do this with 2 rectangles.
	 *
	 * First, draw from cursor to end of line, with the height of the rectangle
	 * being as large as it needs to be until it reaches the end of the screen:
	 * $[ foo bar baz]
	 * m[ore foo     ]END
	 *
	 *
	 * Second, draw from width=0 to the cursor's x coordinate, and a height
	 * also being as large as it needs to be:
	 * $
	 * [m]              END
	 */
	clear_cells(cursor_position.x, cursor_position.y,
						 width_chars(font_data.width) - cursor_position.x,
						 height_chars(font_data.height) - cursor_position.y + 1, font_data.width,
						 font_data.height, foreground, background, *font_data.font);
	clear_cells(0, cursor_position.y, cursor_position.x-1,
						 height_chars(font_data.height) - cursor_position.y + 1, font_data.width,
						 font_data.height, foreground, background, *font_data.font);
}

// Consistent with the define found in console.c.
#define BACKSPACE 0x100

void
vga_write_char(int c, uint32_t foreground, uint32_t background)
{
	struct font_data_8x16 font_data = { 8, 16, &font_default };
	// Past last line.
	if (fb_char_index >= WIDTH * height_chars(font_data.height) ||
	// Last line and newline
			((fb_char_index >= (WIDTH * (height_chars(font_data.height) - 1))) &&
			 c == '\n')) {
		vga_scroll(WIDTH, HEIGHT, font_data.width, font_data.height, font_default);
	} else if (c == '\n') {
		vga_write_newline(WIDTH, font_data.width, HEIGHT);
	} else if (c == '\r') {
		vga_write_carriage_return(WIDTH, font_data.width);
	} else if (c == '\t') {
		vga_write_tab(WIDTH, font_data.width, HEIGHT);
	} else if (c == '\b' || c == BACKSPACE) {
		vga_backspace(font_data.width, font_data.height, background);
	} else {
		render_font_glyph(c,
											pixel_count_to_char_x_coord(fb_char_index, font_data.width) *
										font_data.width,
											pixel_count_to_char_y_coord(fb_char_index, font_data.width) *
										font_data.height,
											font_data.width, font_data.height, *font_data.font, foreground,
											background);
		uint32_t x = pixel_count_to_char_x_coord(fb_char_index, font_data.width);
		uint32_t y = pixel_count_to_char_y_coord(fb_char_index, font_data.width);
		damage_tracking_data.data[x][y] = (char)c;
		damage_tracking_data.fg[x][y] = foreground;
		damage_tracking_data.bg[x][y] = background;
		cursor_position = (struct CursorPosition){x, y};
		fb_char_index += font_data.width;
	}
	if (c == BACKSPACE) {
		uartputc('\b');
		uartputc(' ');
		uartputc('\b');
	} else {
		uartputc(c);
	}
}
