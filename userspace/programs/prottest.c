#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#ifdef __clang__
#define OPTNONE __attribute__((optnone))
#else
#define OPTNONE __attribute__((optimize("O0")))
#endif
static void
nulltest(void)
{
	volatile int *ptr = NULL;
	*ptr = 6;
	assert(*ptr == 6);
}
int
main(void)
{
	nulltest();
	fprintf(stderr, "Tests passed! (This is not a good thing!)\n");
	exit(0);
}
