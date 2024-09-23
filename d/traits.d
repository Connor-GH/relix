module traits;
enum isPointer(T) = is(T == U*, U);
