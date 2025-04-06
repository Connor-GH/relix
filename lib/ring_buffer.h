#pragma once
#include <stddef.h>

struct ring_buf {
	size_t nread;
	size_t nwrite;
	size_t size;
	char *data;
};

enum {
	RING_BUFFER_UNINITIALIZED = -1,
	RING_BUFFER_SUCCESS = 0,
	RING_BUFFER_OOM = 1,
};

struct ring_buf *
ring_buffer_create(size_t nbytes, void *(*allocator)(size_t));
int
ring_buffer_push(struct ring_buf *rb, char *data, size_t n);
int
ring_buffer_pop(struct ring_buf *rb, char *data, size_t n);
void
ring_buffer_destroy(struct ring_buf *rb, void (*deallocator)(void *));
