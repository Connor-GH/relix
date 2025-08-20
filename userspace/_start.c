#include <stdlib.h>
#include <string.h>
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
	optind = 1;
	opterr = 1;

	startup();
	atexit(cleanup);
//  Standard says that main can't have a prototype.
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
