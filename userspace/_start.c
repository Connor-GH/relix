#include <unistd.h>
extern int
main(int argc, char **argv /*, char **envp */);

void
_start(int argc, char **argv /*, char **envp */)
{
	exit(main(argc, argv));
}
