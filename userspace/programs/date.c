#include <stdio.h>
#include <date.h>
#include <ext.h>
#include <ext.h>

int
main(void)
{
	struct rtcdate r;

	if (date(&r) != 0) {
		perror("date");
	}
	printf("%04d-%02d-%02d %02d:%02d:%02d\n", r.year, r.month, r.day, r.hour,
				 r.minute, r.second);
	return 0;
}
