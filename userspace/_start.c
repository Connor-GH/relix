#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
extern int
main(int argc, char **argv /*, char **envp */);

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
	exit(main(argc, argv));
}
