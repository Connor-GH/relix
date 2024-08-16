#include <stdio.h>
#include <unistd.h>

int
main(int argc, const char *argv[])
{
	for (int i = 1; i < argc; i++)
		fprintf(stdout, "%s%s", argv[i], i + 1 < argc ? " " : "\n");
	return 0;
}
