#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	struct utsname utsname;

	bool sflag = false;
	bool nflag = false;
	bool mflag = false;
	bool vflag = false;
	bool rflag = false;
	int c;
	while ((c = getopt(argc, argv, "snmvra")) != -1) {
		switch (c) {
		case 's':
			sflag = true;
			break;
		case 'n':
			nflag = true;
			break;
		case 'm':
			mflag = true;
			break;
		case 'v':
			vflag = true;
			break;
		case 'r':
			rflag = true;
			break;
		case 'a':
			sflag = nflag = mflag = vflag = rflag = true;
			break;
		default:
			break;
		}
	}
	if (!(sflag && nflag && mflag && vflag && rflag)) {
		sflag = true;
	}
	if (uname(&utsname) < 0) {
		perror("uname");
		exit(EXIT_FAILURE);
	}
	if (sflag) {
		printf("%s ", utsname.sysname);
	}
	if (nflag) {
		printf("%s ", utsname.nodename);
	}
	if (rflag) {
		printf("%s ", utsname.release);
	}
	if (vflag) {
		printf("%s ", utsname.version);
	}
	if (mflag) {
		printf("%s ", utsname.machine);
	}
	printf("\n");
	return 0;
}
