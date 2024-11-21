#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
extern int
main(int argc, char **argv /*, char **envp */);

char **environ;
void
_start(int argc, char **argv , char **envp)
{
	environ = envp;
	optind = 1;
	opterr = 1;
	exit(main(argc, argv));
}
