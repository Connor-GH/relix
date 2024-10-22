module kernel_assert;
extern(C) @nogc:
import console;
import compiler_info;
	// get around assert bug that existed in betterC from versions <2.110
static if (vendor == Vendor.digitalMars && version_major == 2 && version_minor < 110) {
	static assert(false, "You need to use DMD >= 2.110.0-beta.1");
}


// used by LDC2
extern(C) void __assert_fail(const char *expression, const char *filename, int line, const char *func) {
	kernel_assert_fail(expression, filename, line, func);
}
@trusted noreturn
kernel_assert_fail(const char *assertion, const char *file, int lineno,
									const char *func);
// the no-op here makes clang happy.

// SAFETY: file, expression, and func are only null if the caller changes them.
@trusted void kernel_assert(bool expr, string expression = "", string file = __FILE__,
		int line = __LINE__, string func = __FUNCTION__)
	in (expression !is null && file !is null && func !is null) {
	if (!expr)
		kernel_assert_fail(expression.ptr, file.ptr, line, func.ptr);
}
