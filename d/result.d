module result;

private struct Empty {};

struct Result(T, E) {
	union DUnion {
		T type;
		E err;
	};
	DUnion result;
	bool is_type = false;
	this(T type) {
		result.type = type;
		is_type = true;
	}
	this(E err) {
		result.err = err;
	}
	static typeof(this) ok(T val) {
		return Result!(T, E)(val);
	}
	static typeof(this) err(E err) {
		return Result!(T, E)(err);
	}
}

R match(R, T, E)(Result!(T, E) res, R function(T) maybe1, R function(E) maybe2) {
	if (res.is_type) {
		return maybe1(res.result.type);
	} else {
		return maybe2(res.result.err);
	}
}
