module traits;
enum isPointer(T) = is(T == U*, U);

template dropPointer(T) {
	static if (is(T U : U*))
		alias dropPointer = U;
	else
		static assert("This is not a pointer");
}

template toPointer(T) {
	alias toPointer = T*;
}
