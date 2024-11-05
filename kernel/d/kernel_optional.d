module kernel_optional;
public import optional;

T unwrap(T)(Option!T o) {
	assert(o.is_ok());
	return __unwrap(o);
}
