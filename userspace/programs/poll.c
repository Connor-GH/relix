#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>

int main(void)
{
	int fd = open("/dev/kbd0", O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	char buffer[8];

	while (1) {
		read(fd, buffer, 4);
		buffer[4] = '\0';
		unsigned char scancode = buffer[0];
		unsigned char keyRelease = (0x80 & scancode);

		scancode = (0x7F & scancode);

		printf("%#x (%c) r=%d\n", scancode, scancode, 0 == keyRelease);
		sleep(100);
	}
	return 0;

}
