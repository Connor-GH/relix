#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define expect_ok_setenv(name, value, overwrite) \
	do {                                           \
		assert(setenv(name, value, overwrite) == 0); \
		assert(getenv(name) != NULL);                \
		assert(strcmp(getenv(name), value) == 0);    \
	} while (0)

#define expect_ok_setenv_noreplace(name, value, overwrite, goodvalue) \
	do {                                                                \
		assert(setenv(name, value, overwrite) == 0);                      \
		assert(getenv(name) != NULL);                                     \
		assert(strcmp(getenv(name), value) != 0 &&                        \
		       strcmp(getenv(name), goodvalue) == 0);                     \
	} while (0)

#define expect_ok_getenv(name, value)         \
	do {                                        \
		assert(getenv(name) != NULL);             \
		assert(strcmp(getenv(name), value) == 0); \
	} while (0)

#define expect_fail_setenv(name, value, over) \
	assert(setenv(name, value, over) != 0)

#define expect_fail_getenv(x) assert(getenv(x) == NULL)

int
main(void)
{
	expect_ok_setenv("TESTENV_VAR1", "0", 1);
	expect_ok_setenv("TESTENV_VAR2", "1", 1);
	expect_ok_setenv("TESTENV_VAR3", "2", 1);
	expect_ok_setenv("TESTENV_VAR4", "3", 1);
	expect_ok_setenv("TESTENV_VAR5", "4", 1);
	expect_ok_setenv("TESTENV_VAR6", "5", 1);
	expect_ok_setenv("TESTENV_VAR7", "6", 1);
	expect_ok_setenv("TESTENV_VAR8", "7", 1);
	expect_ok_setenv("TESTENV_VAR9", "8", 1);
	expect_ok_setenv("TESTENV_VAR10", "9", 1);
	expect_ok_setenv("TESTENV_VAR11", "10", 1);
	expect_ok_setenv("TESTENV_VAR12", "11", 1);

	expect_ok_setenv(
		"A",
		"LONG_ENV_VAR_VALUE________"
		"______________________________________________________________________"
		"______________________________________________________________________"
		"________________________________________________________________end",
		1);

	// Need to specify if you want to overwrite.
	expect_ok_setenv_noreplace("TESTENV_VAR1", "1", 0, "0");
	expect_ok_setenv_noreplace("TESTENV_VAR2", "2", 0, "1");
	expect_ok_setenv_noreplace("TESTENV_VAR3", "3", 0, "2");
	expect_ok_setenv_noreplace("TESTENV_VAR4", "4", 0, "3");
	expect_ok_setenv_noreplace("TESTENV_VAR5", "5", 0, "4");
	expect_ok_setenv_noreplace("TESTENV_VAR6", "6", 0, "5");
	expect_ok_setenv_noreplace("TESTENV_VAR7", "7", 0, "6");
	expect_ok_setenv_noreplace("TESTENV_VAR8", "8", 0, "7");
	expect_ok_setenv_noreplace("TESTENV_VAR9", "9", 0, "8");
	expect_ok_setenv_noreplace("TESTENV_VAR10", "10", 0, "9");
	expect_ok_setenv_noreplace("TESTENV_VAR11", "11", 0, "10");

	expect_ok_getenv("TESTENV_VAR1", "0");
	expect_ok_getenv("TESTENV_VAR2", "1");
	expect_ok_getenv("TESTENV_VAR3", "2");
	expect_ok_getenv("TESTENV_VAR4", "3");
	expect_ok_getenv("TESTENV_VAR5", "4");
	expect_ok_getenv("TESTENV_VAR6", "5");
	expect_ok_getenv("TESTENV_VAR7", "6");
	expect_ok_getenv("TESTENV_VAR8", "7");
	expect_ok_getenv("TESTENV_VAR9", "8");
	expect_ok_getenv("TESTENV_VAR10", "9");
	expect_ok_getenv("TESTENV_VAR11", "10");

	// Try to set val with illegal name.
	expect_fail_setenv("NAME=", "1", 1);
	expect_fail_setenv("N=M", "1", 1);
	expect_fail_setenv("N=M", "=1", 1);

	// Weird value that we still need to support.
	expect_ok_setenv("NAME", "=1", 1);
	expect_ok_getenv("NAME", "=1");

	expect_fail_getenv("N=M");
	expect_fail_getenv("__this_should_not_exist_and_we_are"
	                   "_in_trouble_if_it_does__");

	printf("TESTS PASSED\n");
	return 0;
}
