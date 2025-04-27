#include "ext.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>



#define NUM_ITERS 100000
int
main(void)
{
	for (size_t i = 0; i < NUM_ITERS; i++) {
		time_t time = uptime();
		// Hitting this condition should only happen when the
		// system has been up for 2.7 hours, which is not
		// reasonable for a test program. This is only
		// here to stop the compiler from optimizing
		// the loop away.
		if (time == NUM_ITERS) {
			printf("This is here only for the "
					"loop to not be optimized out\n");
		}
	}
	return 0;
}
