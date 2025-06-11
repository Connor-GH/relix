#include <fcntl.h>
#include <gui.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

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
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	if (ioctl(fd, FBIOCGET_VSCREENINFO, &screeninfo)) {
		perror("ioctl");
		close(fd);
		exit(EXIT_FAILURE);
	}
	close(fd);
	libgui_fill_rect_ptr(
		fb, &(struct rectangle){ 0, 0, screeninfo.xres, screeninfo.yres - 40 },
		0x666666);
	libgui_fill_rect_ptr(
		fb,
		&(struct rectangle){
			.x = 0, .y = screeninfo.yres - 40, .xlen = screeninfo.xres, .ylen = 40 },
		0x123456);
	libgui_fini(fb);
	return 0;
}
