#include <stdlib.h>
#include <gui.h>
#include <stdio.h>

int main(void)
{
	void *fb = libgui_init("/dev/fb0");
	if (fb == NULL) {
		perror("libgui_init");
		exit(EXIT_FAILURE);
	}
	libgui_fill_rect_ptr(fb, &(struct rectangle){0, 0, 640, 440}, 0x666666);
	libgui_fill_rect_ptr(fb, &(struct rectangle){.x = 0, .y = 440, .xlen = 640, .ylen = 40}, 0x123456);
	libgui_fini(fb);
	return 0;
}
