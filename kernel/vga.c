/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "vga.h"
#include "boot/multiboot2.h"
#include "console.h"
#include "font.h"
#include "font_terminus.h"
#include "macros.h"
#include "memlayout.h"
#include "uart.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TAB_WIDTH 4
static struct fb_rgb fb_rgb = { 0 };
static struct multiboot_tag_framebuffer_common fb_common = { 0 };
static void *fb;

// TODO make this take a dynamic font size?
// i.e. remove the "8" and "16". We should get
// this from a font file anyways.
#define FONT_WIDTH 8
#define FONT_HEIGHT 16

struct DamageTracking {
	uint32_t fg[SCREEN_WIDTH / FONT_WIDTH][SCREEN_HEIGHT / FONT_HEIGHT];
	uint32_t bg[SCREEN_WIDTH / FONT_WIDTH][SCREEN_HEIGHT / FONT_HEIGHT];
	char data[SCREEN_WIDTH / FONT_WIDTH][SCREEN_HEIGHT / FONT_HEIGHT];
};

struct CursorPosition {
	uint32_t x;
	uint32_t y;
};

static struct DamageTracking damage_tracking_data = { 0 };
static struct CursorPosition cursor_position = { 0, 0 };

/*
 * This init function needs these 3 parameters
 * because at some point during kernel bring-up,
 * the pointers get invalidated. We store them using
 * copies here.
 *
 * INVARIANT: tag is not NULL.
 */
void
vga_init(struct multiboot_tag_framebuffer *tag)
{
	fb_rgb = tag->rgb;
	fb_common = tag->common;
	fb = IO2V(fb_common.framebuffer_addr);
}

// The color is in hex: 0xRRGGBB
static __always_inline void
vga_write_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	if (x > fb_common.framebuffer_width || y > fb_common.framebuffer_height) {
		return;
	}

	size_t pixel_offset =
		fb_common.framebuffer_pitch * y + (fb_common.framebuffer_bpp / 8) * x;
	multiboot_uint32_t *pixel = fb + pixel_offset;
	*pixel = color;
}

// fb_char_index counts all pixels for width,
// but only the bottom pixels in a font for height. So for
// a 640x480 screen, that is 640 * (480/font_height).
// You index into the width by (fb_char_index % width) / font_width
// You index into the height by (fb_char_index % width)
static uint32_t fb_char_index = 0;

static uint32_t
fb_char_index_to_char_x_coord(uint32_t char_index, uint8_t font_width)
{
	return (char_index % SCREEN_WIDTH) / font_width;
}

static uint32_t
fb_char_index_to_char_y_coord(uint32_t char_index)
{
	return (char_index / SCREEN_WIDTH);
}

static uint32_t
x_y_to_fb_char_index(uint32_t x, uint32_t y, uint8_t font_width)
{
	return (y * SCREEN_WIDTH) + (x * font_width);
}

static void
render_font_glyph(uint8_t character, uint32_t x, uint32_t y, uint8_t font_width,
                  uint8_t font_height,
                  const uint8_t font[static 256][font_height],
                  uint32_t foreground, uint32_t background)
{
	for (uint8_t i = 0; i < font_height; i++) {
		for (uint8_t j = 0; j < font_width; j++) {
			const uint32_t adjusted_x = x + j;
			const uint32_t adjusted_y = y + i;
			if (font[character][i] & (1U << j)) {
				vga_write_pixel(adjusted_x, adjusted_y, foreground);
			} else {
				vga_write_pixel(adjusted_x, adjusted_y, background);
			}
		}
	}
}

static uint32_t
screen_width_in_chars(uint8_t width)
{
	return SCREEN_WIDTH / width;
}

static uint32_t
screen_height_in_chars(uint8_t height)
{
	return SCREEN_HEIGHT / height;
}

void
ansi_set_cursor_location(uint16_t x, uint16_t y)
{
	ansi_set_cursor_location_x(x);
	ansi_set_cursor_location_y(y);
}

void
ansi_set_cursor_location_x(uint16_t x)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	fb_char_index = x_y_to_fb_char_index(x, cursor_position.y, font_data.width);
	cursor_position = (struct CursorPosition){ x, cursor_position.y };
}

void
ansi_set_cursor_location_y(uint16_t y)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	fb_char_index = x_y_to_fb_char_index(cursor_position.x, y, font_data.width);
	cursor_position = (struct CursorPosition){ cursor_position.x, y };
}
void
ansi_set_cursor_location_up(uint16_t by)
{
	ansi_set_cursor_location(0, saturating_sub(cursor_position.y, by, 0));
}

static void
vga_write_carriage_return(uint32_t fb_width, uint8_t font_width)
{
	ansi_set_cursor_location_x(0);
}

void
ansi_erase_from_cursor_to_beginning_of_line(void)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	uint32_t background =
		damage_tracking_data.bg[cursor_position.x][cursor_position.y];
	uint32_t foreground =
		damage_tracking_data.fg[cursor_position.x][cursor_position.y];
	clear_cells(0, cursor_position.y, cursor_position.x, 1, font_data.width,
	            font_data.height, foreground, background, *font_data.font);
}

static void
vga_write_newline(uint32_t fb_width, uint8_t font_width)
{
	if (fb_char_index % fb_width == 0) {
		fb_char_index += fb_width;
	} else {
		fb_char_index = ROUND_UP(fb_char_index, fb_width);
	}
}

static void
vga_write_tab(uint32_t fb_width, uint8_t font_width)
{
	fb_char_index += TAB_WIDTH * font_width;
}

static void
move_cursor(struct CursorPosition *position, uint32_t from_x, uint32_t from_y,
            uint32_t to_x, uint32_t to_y)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	// Erase the previous cursor.
	render_font_glyph(' ', to_x * font_data.width, to_y * font_data.height,
	                  font_data.width, font_data.height, *font_data.font,
	                  damage_tracking_data.bg[position->x][position->y],
	                  damage_tracking_data.fg[position->x][position->y]);
	// Create a new cursor at (to_x, to_y).
	render_font_glyph(damage_tracking_data.data[from_x][from_y],
	                  from_x * font_data.width, from_y * font_data.height,
	                  font_data.width, font_data.height, *font_data.font,
	                  damage_tracking_data.fg[from_x][from_y],
	                  damage_tracking_data.bg[from_x][from_y]);
	// Update our internal position of the cursor.
	cursor_position = (struct CursorPosition){ to_x, to_y };
}

static void
vga_backspace(struct font_data_8x16 *font_data, struct CursorPosition *position,
              uint32_t foreground, uint32_t background)
{
	// We are in the very corner of the screen.
	if (fb_char_index <= 0) {
		return;
	}
	fb_char_index -= font_data->width;
	vga_write_char(' ', foreground, background);

	uint32_t x = fb_char_index_to_char_x_coord(fb_char_index, font_data->width);
	uint32_t y = fb_char_index_to_char_y_coord(fb_char_index);
	move_cursor(position, x, y, x - 1, y);

	fb_char_index -= font_data->width;
}
// This scrolling algorithm is *way* more complex than it should've been.
// The good news is that when using damage tracking for every char,
// the speedup is around 2x.
static void
vga_scroll(uint32_t fb_width, uint32_t fb_height, uint8_t font_width,
           uint8_t font_height, const uint8_t (*font)[])
{
	uint32_t fb_in_bytes = fb_height * fb_width;

	// Initiate scroll.
	for (uint32_t i = (fb_width * font_height); i < fb_in_bytes;
	     i += font_width) {
		uint32_t x = fb_char_index_to_char_x_coord(i, font_width);
		uint32_t y = fb_char_index_to_char_y_coord(i / font_height);
		// We "have" to scroll on these chars.
		if (!(damage_tracking_data.data[x][y] ==
		        damage_tracking_data.data[x][y - 1] &&
		      damage_tracking_data.fg[x][y] == damage_tracking_data.fg[x][y - 1] &&
		      damage_tracking_data.bg[x][y] == damage_tracking_data.bg[x][y - 1])) {
			render_font_glyph(damage_tracking_data.data[x][y], x * font_width,
			                  (y - 1) * font_height, font_width, font_height, font,
			                  damage_tracking_data.fg[x][y],
			                  damage_tracking_data.bg[x][y]);
			damage_tracking_data.data[x][y - 1] = damage_tracking_data.data[x][y];
			damage_tracking_data.fg[x][y - 1] = damage_tracking_data.fg[x][y];
			damage_tracking_data.bg[x][y - 1] = damage_tracking_data.bg[x][y];
		}
	}
	const uint32_t height = screen_height_in_chars(font_height);
	const uint32_t width = screen_width_in_chars(font_width);

	// Clear the last row.
	for (int i = 0; i < width; i++) {
		damage_tracking_data.data[i][height - 1] = ' ';
		damage_tracking_data.fg[i][height - 1] = VGA_COLOR_WHITE;
		damage_tracking_data.bg[i][height - 1] = VGA_COLOR_BLACK;

		clear_cells(i, height - 1, 1, 1, font_width, font_height, VGA_COLOR_WHITE,
		            VGA_COLOR_BLACK, font);
	}
	cursor_position = (struct CursorPosition){ 0, height - 1 };

	vga_write_carriage_return(fb_width, font_width);
}

void
vga_reset_char_index(void)
{
	fb_char_index = 0;
}

void
clear_cells(uint32_t x, uint32_t y, uint32_t x_len, uint32_t y_len,
            uint8_t font_width, uint8_t font_height, uint32_t foreground,
            uint32_t background, const uint8_t (*font)[])
{
	const uint32_t height = screen_height_in_chars(font_height);
	const uint32_t width = screen_width_in_chars(font_width);
	if (x > width || y > height) {
		return;
	}
	for (uint32_t i = 0; i < x_len; i++) {
		for (uint32_t j = 0; j < y_len; j++) {
			render_font_glyph(' ', (i + x) * font_width, (j + y) * font_height,
			                  font_width, font_height, font, foreground, background);
			damage_tracking_data.data[i][j] = ' ';
			damage_tracking_data.fg[i][j] = foreground;
			damage_tracking_data.bg[i][j] = background;
		}
	}
}

void
ansi_erase_from_cursor_to_beginning_of_screen(void)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	uint32_t background =
		damage_tracking_data.bg[cursor_position.x][cursor_position.y];
	uint32_t foreground =
		damage_tracking_data.fg[cursor_position.x][cursor_position.y];
	/*
	 * Observe the following situation where we want to erase from '$' to 'END':
	 * stuff
	 * foo bar $ baz
	 * more foo       END
	 *
	 * You can do this with 2 rectangles.
	 *
	 * First, clear from cursor to beginning of line.
	 * stuff
	 * [foo bar ]$baz
	 * more foo       END
	 *
	 *
	 * Second, clear a rectangle from y-1 to the beginning of the screen, with the
	 * width of the screen.
	 * [stuff        ]
	 *        $ baz
	 *                END
	 */
	clear_cells(0, cursor_position.y, cursor_position.x, 1, font_data.width,
	            font_data.height, foreground, background, *font_data.font);
	clear_cells(0, 0, screen_width_in_chars(font_data.width), cursor_position.y,
	            font_data.width, font_data.height, foreground, background,
	            *font_data.font);
}

void
ansi_erase_from_cursor_to_end_of_screen(void)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	uint32_t background =
		damage_tracking_data.bg[cursor_position.x][cursor_position.y];
	uint32_t foreground =
		damage_tracking_data.fg[cursor_position.x][cursor_position.y];
	/*
	 * Observe the following situation where we want to erase from '$' to 'END':
	 * $ foo bar baz
	 * more foo       END
	 *
	 * You can do this with 2 rectangles.
	 *
	 * First, clear from cursor to end of line.
	 * $[ foo bar baz]
	 * more foo       END
	 *
	 *
	 * Second, clear a rectangle from y+1 to the end of the screen, with the
	 * width of the screen.
	 * $
	 * [             ]END
	 */
	clear_cells(cursor_position.x, cursor_position.y,
	            screen_width_in_chars(font_data.width) - cursor_position.x, 1,
	            font_data.width, font_data.height, foreground, background,
	            *font_data.font);
	clear_cells(0, cursor_position.y + 1, screen_width_in_chars(font_data.width),
	            screen_height_in_chars(font_data.height) - cursor_position.y + 1,
	            font_data.width, font_data.height, foreground, background,
	            *font_data.font);
}

void
ansi_erase_from_cursor_to_end_of_line(void)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	uint32_t background =
		damage_tracking_data.bg[cursor_position.x][cursor_position.y];
	uint32_t foreground =
		damage_tracking_data.fg[cursor_position.x][cursor_position.y];

	clear_cells(cursor_position.x, cursor_position.y,
	            screen_width_in_chars(font_data.width) - cursor_position.x, 1,
	            font_data.width, font_data.height, foreground, background,
	            *font_data.font);
}

// Consistent with the define found in console.c.
#define BACKSPACE 0x100

void
vga_write_char(int c, uint32_t foreground, uint32_t background)
{
	struct font_data_8x16 font_data = { FONT_WIDTH, FONT_HEIGHT, &font_termplus };
	// Past last line.
	if (fb_char_index >=
	      SCREEN_WIDTH * screen_height_in_chars(font_data.height) ||
	    // Last line and newline
	    ((fb_char_index >=
	      (SCREEN_WIDTH * (screen_height_in_chars(font_data.height) - 1))) &&
	     c == '\n')) {
		vga_scroll(SCREEN_WIDTH, SCREEN_HEIGHT, font_data.width, font_data.height,
		           font_termplus);
	} else {
		switch (c) {
		case '\n': {
			// move_cursor requires the old x and y values.
			uint32_t old_x =
				fb_char_index_to_char_x_coord(fb_char_index, font_data.width);
			uint32_t old_y = fb_char_index_to_char_y_coord(fb_char_index);

			vga_write_newline(SCREEN_WIDTH, font_data.width);

			uint32_t x =
				fb_char_index_to_char_x_coord(fb_char_index, font_data.width);
			uint32_t y = fb_char_index_to_char_y_coord(fb_char_index);
			move_cursor(&cursor_position, old_x, old_y, x, y);
			break;
		}
		case '\r':
			vga_write_carriage_return(SCREEN_WIDTH, font_data.width);
			break;
		case '\t':
			// TODO: handle backspace on a tab character.
			vga_write_tab(SCREEN_WIDTH, font_data.width);
			break;
		case '\b':
		case BACKSPACE:
			vga_backspace(&font_data, &cursor_position, foreground, background);
			break;
		default:;
			uint32_t x =
				fb_char_index_to_char_x_coord(fb_char_index, font_data.width);
			uint32_t y = fb_char_index_to_char_y_coord(fb_char_index);
			render_font_glyph(c, x * font_data.width, y * font_data.height,
			                  font_data.width, font_data.height, *font_data.font,
			                  foreground, background);
			damage_tracking_data.data[x][y] = (char)c;
			damage_tracking_data.fg[x][y] = foreground;
			damage_tracking_data.bg[x][y] = background;
			// We need to update the char index.
			fb_char_index += font_data.width;
			// There is the edge case that we are at the last char on the screen.
			// In that case, we need to draw the cursor on the next line.
			uint32_t to_x =
				fb_char_index_to_char_x_coord(fb_char_index, font_data.width);
			uint32_t to_y = fb_char_index_to_char_y_coord(fb_char_index);
			move_cursor(&cursor_position, x, y, to_x, to_y);
			break;
		}
	}
	if (c == BACKSPACE) {
		uartputc('\b');
		uartputc(' ');
		uartputc('\b');
	} else {
		uartputc(c);
	}
}
