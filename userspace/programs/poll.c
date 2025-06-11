#include "kernel/drivers/ps2mouse.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
	int fd = open("/dev/mouse0", O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	uint8_t mouse_data[3];
	bool left = false;
	bool right = false;
	bool middle = false;

	while (1) {
		if (read(fd, mouse_data, 3) < 0) {
			continue;
		}

		int32_t delta_x = mouse_data[1] |
		                  ((mouse_data[0] & (1 << 4)) ? 0xFFFFFF00 : 0);
		int32_t delta_y = mouse_data[2] |
		                  ((mouse_data[0] & (1 << 5)) ? 0xFFFFFF00 : 0);

		// Mouse x/y overflow (bits 6 and 7)
		if ((mouse_data[0] & 0x80) != 0 || (mouse_data[0] & 0x40) != 0) {
			delta_x = 0;
			delta_y = 0;
			continue;
		}

		if ((mouse_data[0] & BUTTON_LEFT)) {
			left = true;
		}
		if ((mouse_data[0] & BUTTON_RIGHT)) {
			right = true;
		}
		if ((mouse_data[0] & BUTTON_MIDDLE)) {
			middle = true;
		}

		printf("dx: %d dy: %d left: %d right: %d mid: %d\n", delta_x, delta_y, left,
		       right, middle);
	}
	return 0;
}
