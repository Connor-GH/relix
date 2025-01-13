#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

/* Possible parameters:
 * - int argc, char **argv, char **envp
 * - int argc, char **argv
 * - void
 */
extern int
main();

void
__fini_stdio(void);
void
cleanup(void)
{
	__fini_stdio();
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
	if (argc >= 1 && strcmp(argv[0], "init") == 0)
		goto skip_init_and_atexit;
	__init_stdio();
	atexit(cleanup);
skip_init_and_atexit:
	exit(main(argc, argv));
}
