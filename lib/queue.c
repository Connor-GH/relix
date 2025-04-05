#include <stdbool.h>
#include <stddef.h>
#include "queue.h"

/*
 * create_queue(allocator)
 * enqueue(q, elem, allocator)
 * dequeue(q, deallocator)
 * free_queue(q, deallocator)
 */

#define QUEUE_T(T) \
struct queue_##T##_node { \
  T data; \
  struct queue_##T##_node *next; \
}; \
\
struct queue_##T { \
  struct queue_##T##_node *front; \
  struct queue_##T##_node *rear; \
  int size; \
}; \
\
struct queue_##T * \
create_queue_##T(void *(*allocator)(size_t)) \
{ \
	struct queue_##T *q = allocator(sizeof(*q)); \
	if (q == NULL) { \
		return NULL; \
	} \
	q->front = NULL; \
	q->rear = NULL; \
	q->size = 0; \
	return q; \
} \
\
int \
is_empty_and_initialized_##T(struct queue_##T *q) \
{ \
	if (q == NULL) { \
		return QUEUE_UNINITIALIZED; \
	} \
	return q->size == 0; \
} \
 \
int \
is_empty_##T(struct queue_##T *q) \
{ \
	return q == NULL || q->size == 0; \
} \
 \
int \
enqueue_##T(struct queue_##T *q, T value, void *(*allocator)(size_t), size_t limit) \
{ \
	if (is_empty_and_initialized_##T(q) == QUEUE_UNINITIALIZED) \
		return QUEUE_UNINITIALIZED; \
\
	if (q->size >= limit) \
		return true; \
 \
	struct queue_##T##_node *new_node = allocator(sizeof(*new_node)); \
	if (new_node == NULL) { \
		return false; \
	} \
	new_node->data = value; \
	new_node->next = NULL; \
\
	if (is_empty_##T(q)) { \
		q->front = new_node; \
	} else { \
		q->rear->next = new_node; \
	} \
	q->rear = new_node; \
	q->size++; \
	return true; \
} \
 \
void \
clean_queue_##T(struct queue_##T *q, void (*deallocator)(void *)) \
{ \
	while (!is_empty_##T(q)) { \
		dequeue_##T(q, NULL, deallocator); \
	} \
} \
 \
int \
dequeue_##T(struct queue_##T *q, T *data, void (*deallocator)(void *)) \
{ \
	if (is_empty_and_initialized_##T(q) == QUEUE_UNINITIALIZED || q->front == NULL) { \
		return false; \
	} \
 \
	struct queue_##T##_node *temp = q->front; \
	T result = temp->data; \
 \
	q->front = q->front->next; \
	if (q->front == NULL) { \
		q->rear = NULL; \
	} \
	if (temp != NULL) \
		deallocator(temp); \
	if ((signed)q->size > 0) \
		q->size--; \
	if (data != NULL) { \
		*data = result; \
	} \
	return true; \
} \
 \
void \
free_queue_##T(struct queue_##T *q, void (*deallocator)(void *)) \
{ \
	clean_queue_##T(q, deallocator); \
	deallocator(q); \
}

QUEUE_T(int)
QUEUE_T(unsigned_char)

