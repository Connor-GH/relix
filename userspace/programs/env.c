#include <stdio.h>
extern char **environ;

int
main(int argc, char **argv)
{
	if (argc == 1) {
		for (int i = 0; environ[i]; i++)
			printf("%s\n", environ[i]);
	}
	return 0;
}
