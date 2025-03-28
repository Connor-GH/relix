#pragma once
#include <stdint.h>

/* Exported to userspace */
struct fb_var_screeninfo {
	uint32_t xres;
	uint32_t yres;
	uint8_t bpp;
};
