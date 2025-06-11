#include <stdio.h>
#include <unistd.h>

int
main(void)
{
	printf("%s\n", ttyname(STDIN_FILENO));
	return 0;
}
