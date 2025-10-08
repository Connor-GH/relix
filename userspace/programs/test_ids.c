#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#define SHOULD_SUCCEED(x) assert((x) >= 0)
#define SHOULD_FAIL(x) assert((x) < 0)

int
main(void)
{
	gid_t list[NGROUPS_MAX + 1];
	SHOULD_SUCCEED(setuid(0));
	// We currently have no supplimentary groups.
	assert(getgroups(NGROUPS_MAX + 1, list) == 0);
	assert(getgid() == 0);
	assert(getegid() == 0);
	printf("TESTS PASSED\n");
	return 0;
}
