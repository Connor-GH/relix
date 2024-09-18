module object;
version (X86_64) {} else {
	alias size_t = uint;
	alias noreturn = typeof(*null);
	alias string = immutable(char)[];
}
// define dummy methods until the assert bug is fixed in dmd
extern(C) void __assert(const char *, const char *, int, const char*) {}
extern(C) void __assert_fail(const char *, const char *, int, const char*) {}


bool __equals(T1, T2)(T1[] lhs, T2[] rhs) {
	static if (!is(typeof(lhs) == typeof(rhs)))
		return false;
	if (lhs is null && rhs is null)
		return true;
	if ((lhs is null && rhs !is null) || (lhs !is null && rhs is null))
		return false;
	if (lhs.length != rhs.length)
		return false;
	static if (is(T1 == string)) {
		for (int i = 0; i < lhs.length; i++) {
			if (lhs[i] != rhs[i])
				return false;
		}
		return true;
	}
	for (int i = 0; i < lhs.length; i++) {
		T1 a = lhs[i];
		if (a != rhs[i]) {
			return false;
		}
		return true;
	}
	return false;
}
