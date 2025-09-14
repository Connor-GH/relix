#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
	// The strings are created twice
	// because basename and dirname
	// can modify their arguments.
	char basename_str1[] = "usr";
	char basename_str2[] = "usr/";
	char basename_str3[] = "";
	char basename_str4[] = "/";
	char basename_str5[] = "//";
	char basename_str6[] = "///";
	char basename_str7[] = "/usr/";
	char basename_str8[] = "/usr/lib";
	char basename_str9[] = "//usr//lib//";
	char basename_str10[] = "/home//dwc//test";
	assert(strcmp(basename(basename_str1), "usr") == 0);
	assert(strcmp(basename(basename_str2), "usr") == 0);
	assert(strcmp(basename(basename_str3), ".") == 0);
	assert(strcmp(basename(basename_str4), "/") == 0);
	assert((strcmp(basename(basename_str5), "/") == 0) ||
	       (strcmp(basename(basename_str5), "//") == 0));
	assert(strcmp(basename(basename_str6), "/") == 0);
	assert(strcmp(basename(basename_str7), "usr") == 0);
	assert(strcmp(basename(basename_str8), "lib") == 0);
	assert(strcmp(basename(basename_str9), "lib") == 0);
	assert(strcmp(basename(basename_str10), "test") == 0);
	char dirname_str1[] = "usr";
	char dirname_str2[] = "usr/";
	char dirname_str3[] = "";
	char dirname_str4[] = "/";
	char dirname_str5[] = "//";
	char dirname_str6[] = "///";
	char dirname_str7[] = "/usr/";
	char dirname_str8[] = "/usr/lib";
	char dirname_str9[] = "//usr//lib//";
	char dirname_str10[] = "/home//dwc//test";
	assert(strcmp(dirname(dirname_str1), ".") == 0);
	assert(strcmp(dirname(dirname_str2), ".") == 0);
	assert(strcmp(dirname(dirname_str3), ".") == 0);
	assert(strcmp(dirname(dirname_str4), "/") == 0);
	assert((strcmp(dirname(dirname_str5), "/") == 0) ||
	       (strcmp(dirname_str5, "//") == 0));
	assert(strcmp(dirname(dirname_str6), "/") == 0);
	assert(strcmp(dirname(dirname_str7), "/") == 0);
	assert(strcmp(dirname(dirname_str8), "/usr") == 0);
	assert(strcmp(dirname(dirname_str9), "//usr") == 0);
	assert(strcmp(dirname(dirname_str10), "/home//dwc") == 0);
	printf("TESTS PASSED\n");
	return 0;
}
