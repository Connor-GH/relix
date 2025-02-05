#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>

#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic push
static void __attribute__((optnone)) __attribute__((optimize("O0")))
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
	return 1;
}
#pragma GCC diagnostic pop
