#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
extern int
main(int argc, char **argv /*, char **envp */);

void
cleanup(void)
{
	// Flushes all output streams.
	fflush(NULL);
}
void
__init_stdio(void);
char **environ;
void
_start(int argc, char **argv, char **envp)
{
	environ = envp;
	assert(environ != NULL);
	optind = 1;
	opterr = 1;
	__init_stdio();
	atexit(cleanup);
	exit(main(argc, argv));
}
