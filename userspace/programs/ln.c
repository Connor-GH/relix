#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
int
main(int argc, char *argv[])
{
  if (argc != 3) {
	fprintf(2, "Usage: ln old new\n");
	exit(0);
  }
  if (link(argv[1], argv[2]) < 0)
	fprintf(2, "link %s %s: failed\n", argv[1], argv[2]);
  exit(0);
}
