#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
extern int
main(int argc, char **argv /*, char **envp */);

char **environ;
void
_start(int argc, char **argv, char **envp)
{
	environ = envp;
	assert(environ != NULL);
	optind = 1;
	opterr = 1;
	exit(main(argc, argv));
}
