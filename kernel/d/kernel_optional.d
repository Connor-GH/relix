module kernel_optional;
public import optional;
import kernel_assert : kernel_assert;

T unwrap(T)(Option!T o) {
	kernel_assert(o.is_ok());
	return __unwrap(o);
}
