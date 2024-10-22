module object;
import traits;
version (X86_64) {} else {
	alias size_t = uint;
	alias noreturn = typeof(*null);
	alias string = immutable(char)[];
}

version (__KERNEL__) {
	import kernel.d.bindings.kernel_string;
} else {
	import userspace.d.bindings.string_;
}

@safe:
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


@system bool __equals(T1, T2)(const T1 t1, const T2 t2)
	if (isPointer!T1 && isPointer!T2)
{
	static if (is(T1 == char *) && is(T2 == char *)) {
		return strncmp(t1, t2, strlen(t1)) == 0;
	} else {
		return t1 == t2;
	}
}
