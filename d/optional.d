module optional;

@safe:
struct Option(T) {
	private T data;
	this(T data) {
		this.data = data;
	}
}

Option!T Some(T)(T value) {
	return Option!T(value);
}

Option!T None(T)() {
	return Option!T(T.init);
}

T unwrap(T)(Option!T o) {
	static if (__traits(isArithmetic, o))
		assert(o.data != T.init);
	return o.data;
}
