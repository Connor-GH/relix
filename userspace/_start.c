#include <unistd.h>
extern int
main(int argc, char **argv /*, char **envp */);

void
_start(int argc, char **argv /*, char **envp */)
{
	optind = 1;
	opterr = 1;
	exit(main(argc, argv));
}
