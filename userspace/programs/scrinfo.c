#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int
main(void)
{
	struct fb_var_screeninfo info;
	int fd = open("/dev/fb0", O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	if (ioctl(fd, FBIOCGET_VSCREENINFO, &info) < 0) {
		perror("ioctl");
		close(fd);
		exit(EXIT_FAILURE);
	}
	printf("Screen: %ux%u %dbpp\n", info.xres, info.yres, info.bpp);
	close(fd);

	return 0;
}
