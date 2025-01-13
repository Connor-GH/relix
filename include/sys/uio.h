#pragma once
#include <stddef.h>
struct iovec {
	void *iov_base;
	size_t iov_len;
};
#ifdef __USER__
ssize_t
writev(int fd, const struct iovec *iov, int iovcnt);
#endif
