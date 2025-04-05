#pragma once
#include <stddef.h>

#define QUEUE_T_HEADER(T) \
/* Returns NULL on error. */ \
struct queue_##T * \
create_queue_##T(void *(*allocator)(size_t)); \
\
/* Enqueues the element. Returns true on success, */ \
/* QUEUE_UNINITIALIZED on the queue being uninitialized, */ \
/* and false if we failed to allocate. */ \
int \
enqueue_##T(struct queue_##T *q, T value, void *(*allocator)(size_t), size_t limit); \
\
/* Dequeue an element. Returns NULL on error. */ \
/* Otherwise, returns a pointer to the element. */ \
int \
dequeue_##T(struct queue_##T *q, T *data, void (*deallocator)(void *)); \
\
void \
free_queue_##T(struct queue_##T *q, void (*deallocator)(void *)); \
\
/* Removes everything in the queue. */ \
void \
clean_queue_##T(struct queue_##T *q, void (*deallocator)(void *));

#define QUEUE_UNINITIALIZED (-1)
#define QUEUE_EMPTY (1)
#define QUEUE_POPULATED (0)

typedef unsigned char unsigned_char;
QUEUE_T_HEADER(int)
QUEUE_T_HEADER(unsigned_char)
