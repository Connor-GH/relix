module kernel_assert;
extern(C) @nogc:
import console;
noreturn
kernel_assert_fail(const char *assertion, const char *file, int lineno,
									const char *func);
// the no-op here makes clang happy.
static void
no_op(){}
void kernel_assert(bool expr) {
	if (expr)
		no_op();
	else
		kernel_assert_fail("", __FILE__, __LINE__, "<func>");
}
