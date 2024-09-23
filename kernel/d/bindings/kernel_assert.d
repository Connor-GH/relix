module kernel_assert;
extern(C) @nogc:
import console;
@trusted noreturn
kernel_assert_fail(const char *assertion, const char *file, int lineno,
									const char *func);
// the no-op here makes clang happy.
@safe static void
no_op(){}

// SAFETY: file, expression, and func are only null if the caller changes them.
@trusted void kernel_assert(bool expr, string expression = "", string file = __FILE__,
		int line = __LINE__, string func = __FUNCTION__)
	in (expression !is null && file !is null && func !is null) {
	if (expr)
		no_op();
	else
		kernel_assert_fail(expression.ptr, file.ptr, line, func.ptr);
}
