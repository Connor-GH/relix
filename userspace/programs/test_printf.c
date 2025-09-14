#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

static char buffer[128] = {};

#define TEST(x, y, str) \
	assert((x) == (y));   \
	assert(strcmp(buffer, str) == 0);

static int
my_vsnprintf(const char *fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vsnprintf(buffer, sizeof(buffer), fmt, listp);
	va_end(listp);
	assert(strnlen(buffer, sizeof(buffer)) == ret);
	printf("(%s)\n", buffer);
	return ret;
}

int
main(void)
{
	printf("Testing printf\n");

	// do we return the correct amount of chars written?
	TEST(my_vsnprintf("%s", ""), 0, "");
	TEST(my_vsnprintf("1"), 1, "1");
	TEST(my_vsnprintf("12"), 2, "12");
	TEST(my_vsnprintf("123"), 3, "123");
	TEST(my_vsnprintf("1234"), 4, "1234");
	TEST(my_vsnprintf("12345"), 5, "12345");

	TEST(my_vsnprintf("1%s", ""), 1, "1");
	TEST(my_vsnprintf("1%s", " "), 2, "1 ");
	TEST(my_vsnprintf("1%s", "  "), 3, "1  ");
	TEST(my_vsnprintf("1%s", "   "), 4, "1   ");
	TEST(my_vsnprintf("1%s", "    "), 5, "1    ");

	TEST(my_vsnprintf("1%s1", ""), 2, "11");
	TEST(my_vsnprintf("1%s12", " "), 4, "1 12");
	TEST(my_vsnprintf("1%s123", "  "), 6, "1  123");
	TEST(my_vsnprintf("1%s1234", "   "), 8, "1   1234");
	TEST(my_vsnprintf("1%s12345", "    "), 10, "1    12345");

	TEST(my_vsnprintf("1%s%s1", "", ""), 2, "11");
	TEST(my_vsnprintf("1%s%s12", " ", ""), 4, "1 12");
	TEST(my_vsnprintf("1%s%s123", "  ", ""), 6, "1  123");
	TEST(my_vsnprintf("1%s%s1234", "   ", ""), 8, "1   1234");
	TEST(my_vsnprintf("1%s%s12345", "    ", ""), 10, "1    12345");

	TEST(my_vsnprintf("1%s%s1", "", " "), 3, "1 1");
	TEST(my_vsnprintf("1%s%s12", " ", " "), 5, "1  12");
	TEST(my_vsnprintf("1%s%s123", "  ", " "), 7, "1   123");
	TEST(my_vsnprintf("1%s%s1234", "   ", " "), 9, "1    1234");
	TEST(my_vsnprintf("1%s%s12345", "    ", " "), 11, "1     12345");

	TEST(my_vsnprintf("%d", 1), 1, "1");
	TEST(my_vsnprintf("%d", 10), 2, "10");
	TEST(my_vsnprintf("%d", 100), 3, "100");
	TEST(my_vsnprintf("%d", 1000), 4, "1000");
	TEST(my_vsnprintf("%d", 10000), 5, "10000");

	TEST(my_vsnprintf("%01d", 1), 1, "1");
	TEST(my_vsnprintf("%02d", 1), 2, "01");
	TEST(my_vsnprintf("%03d", 1), 3, "001");
	TEST(my_vsnprintf("%04d", 1), 4, "0001");
	TEST(my_vsnprintf("%05d", 1), 5, "00001");

	TEST(my_vsnprintf("%1d", 1), 1, "1");
	TEST(my_vsnprintf("%2d", 1), 2, " 1");
	TEST(my_vsnprintf("%3d", 1), 3, "  1");
	TEST(my_vsnprintf("%4d", 1), 4, "   1");
	TEST(my_vsnprintf("%5d", 1), 5, "    1");

	TEST(my_vsnprintf("%-1d", 1), 1, "1");
	TEST(my_vsnprintf("%-2d", 1), 2, "1 ");
	TEST(my_vsnprintf("%-3d", 1), 3, "1  ");
	TEST(my_vsnprintf("%-4d", 1), 4, "1   ");
	TEST(my_vsnprintf("%-5d", 1), 5, "1    ");

	TEST(my_vsnprintf("0x%x", 0x1234567U), 9, "0x1234567");
	TEST(my_vsnprintf("%#x", 0x1234567U), 9, "0x1234567");
	TEST(my_vsnprintf("%#05x", 0x1234U), 6, "0x1234");
	TEST(my_vsnprintf("%#06x", 0x1234U), 6, "0x1234");
	TEST(my_vsnprintf("%#07x", 0x1234U), 7, "0x01234");
	TEST(my_vsnprintf("%#08x", 0x1234U), 8, "0x001234");

	// Test whether we get the right result for
	// %[#]?[xbo] with zero.
	TEST(my_vsnprintf("%x", 0), 1, "0");
	TEST(my_vsnprintf("%#x", 0), 1, "0");

	TEST(my_vsnprintf("%b", 0), 1, "0");
	TEST(my_vsnprintf("%#b", 0), 1, "0");

	TEST(my_vsnprintf("%o", 0), 1, "0");
	TEST(my_vsnprintf("%#o", 0), 1, "0");

	// Test whether we get the right result for
	// %[#]?[xbo] with nonzero.
	TEST(my_vsnprintf("%x", 1), 1, "1");
	TEST(my_vsnprintf("%#x", 1), 3, "0x1");

	TEST(my_vsnprintf("%b", 1), 1, "1");
	TEST(my_vsnprintf("%#b", 1), 3, "0b1");

	TEST(my_vsnprintf("%o", 1), 1, "1");
	TEST(my_vsnprintf("%#o", 1), 2, "01");

	assert(snprintf(NULL, SIZE_MAX, "%s%s%s", "1", "2", "3") == 3);
	printf("printf test PASSED\n");

	return 0;
}
