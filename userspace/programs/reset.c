#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

int
main(void)
{
	struct termios termios = {
		.c_iflag = ~(IXOFF | INPCK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
			ICRNL | IXON | IGNPAR) | IGNBRK,
		.c_oflag = ~OPOST,
		.c_lflag = ~(ECHOE | ECHOK | ECHONL | ICANON | ISIG | IEXTEN |
			NOFLSH | TOSTOP) | ECHO,
		.c_cflag = ~(CSIZE | PARENB) | CS8 | CREAD,
		.c_cc = {
			// Other elements are _POSIX_VDISABLE (0).
			[VMIN] = 1, [VTIME] = 0,
		},
		.c_ospeed = B19200,
		.c_ispeed = B19200,
	};
	int fd = open("/dev/tty0", O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	if (tcsetattr(fd, TCSANOW, &termios) < 0) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return 0;
}
