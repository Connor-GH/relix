#include "ring_buffer.h"
#include <stddef.h>


struct ring_buf *
ring_buffer_create(size_t nbytes, void *(*allocator)(size_t))
{
	struct ring_buf *ring_buffer;
	if ((ring_buffer = allocator(sizeof(*ring_buffer))) == NULL)
		return NULL;
	if (nbytes == 0)
		return NULL;
	ring_buffer->nread = 0;
	ring_buffer->nwrite = 0;
	ring_buffer->data = allocator(nbytes);
	if (ring_buffer->data == NULL)
		return NULL;
	ring_buffer->size = nbytes;
	return ring_buffer;
}

int
ring_buffer_push(struct ring_buf *rb, char *data, size_t n)
{
	if (rb == NULL)
		return RING_BUFFER_UNINITIALIZED;

	for (size_t i = 0; i < n; i++) {
		rb->data[rb->nwrite++ % rb->size] = data[i];
	}
	return RING_BUFFER_SUCCESS;
}

int
ring_buffer_pop(struct ring_buf *rb, char *data, size_t n)
{
	if (rb == NULL)
		return RING_BUFFER_UNINITIALIZED;

	for (size_t i = 0; i < n; i++) {
		if (rb->nread == rb->nwrite)
			break;
		data[i] = rb->data[rb->nread++ % rb->size];
	}
	return RING_BUFFER_SUCCESS;
}

void
ring_buffer_destroy(struct ring_buf *rb, void (*deallocator)(void *))
{
	if (rb) {
		if (rb->data) {
			deallocator(rb->data);
		}
		deallocator(rb);
	}
}


