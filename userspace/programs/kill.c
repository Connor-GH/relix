#include <stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
int
main(int argc, char **argv)
{
  int i;

  if (argc < 2) {
	fprintf(2, "usage: kill pid...\n");
	exit(0);
  }
  for (i = 1; i < argc; i++)
	kill(atoi(argv[i]));
  exit(0);
}
