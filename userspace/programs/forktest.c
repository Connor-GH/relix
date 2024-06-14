// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include <user.h>
#include <stdlib.h>

#define N 1000


void
forktest(void)
{
  int n, pid;

  fprintf(1, "fork test\n");

  for (n = 0; n < N; n++) {
	pid = fork();
	if (pid < 0)
	  break;
	if (pid == 0)
	  exit(0);
  }

  if (n == N) {
	fprintf(1, "fork claimed to work N times!\n", N);
	exit(0);
  }

  for (; n > 0; n--) {
	if (wait(NULL) < 0) {
	  fprintf(1, "wait stopped early\n");
	  exit(0);
	}
  }

  if (wait(NULL) != -1) {
	fprintf(1, "wait got too many\n");
	exit(0);
  }

  fprintf(1, "fork test OK\n");
}

int
main(void)
{
  forktest();
  exit(0);
}
