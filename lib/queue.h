#pragma once
#include <stddef.h>
#include <stdint.h>

#define QUEUE_T_HEADER(T)                                                     \
	/* Returns NULL on error. */                                                \
	struct queue_##T *create_queue_##T(void *(*allocator)(size_t));             \
                                                                              \
	/* Enqueues the element. Returns true on success, */                        \
	/* QUEUE_UNINITIALIZED on the queue being uninitialized, */                 \
	/* and false if we failed to allocate. */                                   \
	int enqueue_##T(struct queue_##T *q, T value, void *(*allocator)(size_t),   \
	                size_t limit);                                              \
                                                                              \
	/* Dequeue an element. Returns true on success. */                          \
	int dequeue_##T(struct queue_##T *q, T *data, void (*deallocator)(void *)); \
                                                                              \
	void free_queue_##T(struct queue_##T *q, void (*deallocator)(void *));      \
                                                                              \
	/* Removes everything in the queue. */                                      \
	void clean_queue_##T(struct queue_##T *q, void (*deallocator)(void *));     \
	int is_empty_##T(struct queue_##T *q);

enum {
	QUEUE_UNINITIALIZED = -1,
	QUEUE_SUCCESS = 0,
	QUEUE_EMPTY = 1,
	QUEUE_OOM = 2,
	QUEUE_FULL = 3,
};

typedef unsigned char unsigned_char;
typedef struct mouse_packet {
	uint8_t data[3];
} mouse_packet;

QUEUE_T_HEADER(int)
QUEUE_T_HEADER(unsigned_char)
QUEUE_T_HEADER(mouse_packet)
