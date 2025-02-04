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
	libgui_fill_rect_ptr(fb, &(struct rectangle){0, 0, 640, 480}, 0xff0000);
	libgui_fini(fb);
	return 0;
}
