#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

int main(void)
{
	char buf[PATH_MAX];
	if (getcwd(buf, sizeof(buf)) == NULL) {
		perror("getcwd");
		exit(EXIT_FAILURE);
	}
	printf("%s\n", buf);
	return 0;
}
