# Syscall documentation

Currently, Relix is a monolithic kernel. This means that nearly everything in `man` page section 2 is implemented directly as a syscall routine. Relix is different in the sense that it tries to implement as many syscalls in terms of one syscall as possible, and does not have a stable syscall ABI. For instance, `open()` used to be implemented as a syscall routine directly, but now it is implemented in terms of `openat()`.

Here is the direct implementation of `open()` from userspace:

```
int
open(const char *pathname, int flags, ...)
{
	mode_t mode;
	va_list listp;
	if ((flags & O_CREAT) == O_CREAT || (flags & O_TMPFILE) == O_TMPFILE) {
		va_start(listp, flags);
		mode = va_arg(listp, mode_t);
		va_end(listp);
	} else {
		mode = 0;
	}
	return __syscall_ret(
		__syscall4(SYS_openat, (long)AT_FDCWD, (long)pathname, flags, mode));
```

Majority of the code for `open()` is just dealing with its variadic arg. Variadic args in syscall interfaces are not a great idea, and this is can be confirmed by POSIX not using them for new functions in `man` page section 2 anymore.

The last line is the most important; reading from the inside-out, it calls `__syscall4`, an internal function which handles syscalls, and then calls `__syscall_ret`. `__syscall_ret` is just a helper function that deals with how Relix returns syscall values. Part of Relix's syscall ABI is that errno values are returned through negative values from the syscall; in other words, `ret >= 0` is success, and `ret < 0` is failure. Additionally, `-ret` is the value that is put in errno if there was a failure. `__syscall4` is simply an inline assembly function that assigns to the proper registers, and then uses the amd64 `syscall` instruction.
