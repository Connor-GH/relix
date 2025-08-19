#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

void __fini_stdio(void);
static void
cleanup(void)
{
	__fini_stdio();
}

void __init_stdio(void);

static void
startup(void)
{
	__init_stdio();
}

char **environ;
void
_start(int argc, char **argv, char **envp)
{
	environ = envp;
	assert(environ != NULL);
	optind = 1;
	opterr = 1;
	if (argc >= 1 && strcmp(argv[0], "init") == 0) {
		goto skip_init_and_atexit;
	}
	startup();
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
