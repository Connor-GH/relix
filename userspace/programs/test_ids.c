#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#define SHOULD_SUCCEED(x) assert((x) >= 0)
#define SHOULD_FAIL(x) assert((x) < 0)

int
main(void)
{
	pid_t mypid = getpid();
	pid_t parentpid = getppid();
	gid_t mygid = getgid();
	gid_t myegid = getegid();
	gid_t list[NGROUPS_MAX + 1];
	SHOULD_SUCCEED(setuid(0));
	// We currently have no supplimentary groups.
	assert(mypid != parentpid);
	assert(getgroups(NGROUPS_MAX + 1, list) == 0);
	// By default, we inherit real and effective group ID
	// from the parent.
	if (mygid == 0) {
		assert(mygid == myegid);
	}
	// By default, session ID and process group ID
	// are the same for the parent and the child.
	assert(getsid(mypid) == getsid(parentpid));
	assert(getpgid(mypid) == getpgid(parentpid));
	SHOULD_SUCCEED(setsid());
	assert(getsid(mypid) != getsid(parentpid));
	SHOULD_SUCCEED(setpgid(mypid, mypid));
	assert(getpgid(mypid) != getpgid(parentpid));

	printf("TESTS PASSED\n");
	return 0;
}
