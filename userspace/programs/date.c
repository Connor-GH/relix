#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int
main(void)
{

	time_t unix_seconds;
	time_t time2;
	if ((time2 = time(&unix_seconds)) == (time_t)-1) {
		perror("time");
		exit(EXIT_FAILURE);
	}
	struct tm *tm = localtime(&unix_seconds);
	if (tm == NULL) {
		perror("localtime");
		exit(EXIT_FAILURE);
	}
	// TODO:
	// - outputs numeric month instead of ASCII month
	// - does not output week day
	// - does not output AM/PM
	fprintf(stdout, "%04d %02d %02d:%02d:%02d %s %d\n",
				tm->tm_mon, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_zone, tm->tm_year);

	return 0;
}
