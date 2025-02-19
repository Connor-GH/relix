#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(void)
{
	struct fb_var_screeninfo info;
	if (ioctl(0, FBIOGET_VSCREENINFO, &info) < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
	printf("Screen: %ux%u %dbpp\n", info.xres, info.yres, info.bpp);

	return 0;
}
