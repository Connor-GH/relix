#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(void)
{
	char *logname = getlogin();

	if (logname == NULL) {
		perror("getlogin");
		exit(EXIT_FAILURE);
	}
	printf("%s\n", logname);
	return 0;
}
