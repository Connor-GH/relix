#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>

static void
nulltest(int zero)
{
	volatile int *ptr = (int *)((size_t)zero);
	*ptr = 6;
	assert(*ptr == 6);
}
int
main(int argc, char **argv)
{
	nulltest(argc-1);
	fprintf(stderr, "Tests passed! (This is not a good thing!)\n");
	return 1;
}
