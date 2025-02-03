#include <stdlib.h>
#include <gui.h>
#include <stdio.h>

int main(void)
{
	FILE *fp = libgui_init("/dev/fb0");
	if (fp == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	libgui_fill_rect_fp(fp, &(struct rectangle){0, 0, 1920, 1080}, 0xff0000);
	libgui_fini(fp);
	return 0;
}
