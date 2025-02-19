#pragma once
#include "vga.h"
#include <stdint.h>

struct fb_var_screeninfo {
	uint32_t xres;
	uint32_t yres;
	uint8_t bpp;
};
