#include <assert.h>
#include <unistd.h>
int
main(void)
{
	assert((int)close(-1) == -1);
	assert((int)close(3) == -1);
	assert((int)close(4) == -1);
	assert((int)close(4294967295U) == -1);
	assert((int)close(-2) == -1);
	assert((int)close(-3) == -1);
	assert((int)close(12) == -1);
	assert((int)close(1200) == -1);
	assert((int)close(2147483647) == -1);
	assert((int)close(-2147483647) == -1);
	return 0;
}
