#pragma once
#include <stdint.h>
#include <sys/types.h>

struct rectangle {
	uint32_t x;
	uint32_t y;
	uint32_t xlen;
	uint32_t ylen;
};
ssize_t libgui_pixel_write(uint32_t x, uint32_t y, uint32_t color);
ssize_t libgui_pixel_write_ptr(void *ptr, uint32_t x, uint32_t y,
                               uint32_t color);
void libgui_fill_rect(const struct rectangle *const rect, uint32_t hex_color);
void libgui_fill_rect_ptr(void *ptr, const struct rectangle *const rect,
                          uint32_t hex_color);
void *libgui_init(const char *const file);
void libgui_fini(void *ptr);
