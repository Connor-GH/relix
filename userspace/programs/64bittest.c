#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int
main(void)
{
	uint64_t x = 18000000000000000000UL;
	int64_t sig_x = 9000000000000000000L;
	char *s = malloc(2 + 18 + 1);
	if (s == NULL) {
		perror("malloc");
		exit(1);
	}
	char *su = malloc(19);
	if (su == NULL) {
		perror("malloc");
		exit(1);
	}
	sprintf(s, "%lu", x);
	sprintf(su, "%ld", sig_x);
	assert(strcmp(s, "18000000000000000000") == 0);
	assert(strcmp(su, "9000000000000000000") == 0);
	free(s);
	free(su);
	printf("ALL TESTS PASSED\n");
	return 0;
}
