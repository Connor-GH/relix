#include "ext.h"
#include <assert.h>
#include <fcntl.h>
#include <gui.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

int
main(void)
{
	void *fb = libgui_init("/dev/fb0");
	struct fb_var_screeninfo screeninfo;
	if (fb == NULL) {
		perror("libgui_init");
		exit(EXIT_FAILURE);
	}
	int fd = open("/dev/fb0", O_RDONLY);
	if (ioctl(fd, FBIOCGET_VSCREENINFO, &screeninfo)) {
		perror("ioctl");
		exit(1);
	}
	close(fd);
	time_t start = uptime();
	assert(screeninfo.yres == 480 && screeninfo.xres == 640);
	for (int i = 0; i < 30 * 10; i++) {
		libgui_fill_rect_ptr(
			fb, &(struct rectangle){ 0, 0, screeninfo.xres, screeninfo.yres },
			0x666666);
		libgui_fill_rect_ptr(
			fb, &(struct rectangle){ 0, 0, screeninfo.xres, screeninfo.yres },
			0x000000);
	}
	time_t diff = uptime() - start;
	printf("fps: %f\n", 300. / (diff / 1000.));
	libgui_fini(fb);
	return 0;
}
