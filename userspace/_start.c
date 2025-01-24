#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

void
__fini_stdio(void);
void
cleanup(void)
{
	__fini_stdio();
}
void
__init_stdio(void);
char *const *environ;
void
_start(int argc, char *const *argv, char *const *envp)
{
	environ = envp;
	assert(environ != NULL);
	optind = 1;
	opterr = 1;
	if (argc >= 1 && strcmp(argv[0], "init") == 0)
		goto skip_init_and_atexit;
	__init_stdio();
	atexit(cleanup);
skip_init_and_atexit:
// Standard says that main can't have a prototype.
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic push
	/* Possible parameters:
 * - int argc, char **argv, char **envp
 * - int argc, char **argv
 * - void
 */
	exit(main(argc, argv));
#pragma GCC diagnostic pop
}
