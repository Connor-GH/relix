module assert_;
extern(C) @nogc void
assert_fail(const char *assertion, const char *file, int lineno, const char *func);
