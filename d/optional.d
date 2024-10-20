module optional;

@safe:
struct Option(T) {
	private T data = void;
	private bool is_none = true;
	@disable this();
	this(T data) {
		this.data = data;
		is_none = false;
	}
	this(typeof(null)) {
		is_none = true;
	}
	bool is_ok() {
		return !this.is_none;
	}
}

Option!T Some(T)(T value) {
	return Option!T(value);
}

Option!T None(T)() {
	return Option!T(null);
}

T __unwrap(T)(Option!T o) {
	return o.data;
}
