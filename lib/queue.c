#include <stdbool.h>
#include <stddef.h>
#include "queue.h"

/*
 * create_queue(allocator)
 * enqueue(q, elem, allocator)
 * dequeue(q, deallocator)
 * free_queue(q, deallocator)
 */

struct queue *
create_queue(void *(*allocator)(size_t))
{
	struct queue *q = allocator(sizeof(*q));
	if (q == NULL) {
		return NULL;
	}
	q->front = NULL;
	q->rear = NULL;
	q->size = 0;
	return q;
}

int
is_empty_and_initialized(struct queue *q)
{
	if (q == NULL) {
		return QUEUE_UNINITIALIZED;
	}
	return q->size == 0;
}

__attribute__((nonnull(1)))
int
is_empty(struct queue *q)
{
	return q->size == 0;
}

size_t
queue_length(struct queue *q)
{
	size_t i = 0;
	for (struct queue_node *qp = q->front; qp != NULL; qp = qp->next, i++)
		;
	return i;
}

int
enqueue(struct queue *q, int value, void *(*allocator)(size_t), size_t limit)
{
	if (is_empty_and_initialized(q) == QUEUE_UNINITIALIZED)
		return QUEUE_UNINITIALIZED;

	if (queue_length(q) >= limit)
		return true;

	struct queue_node *new_node = allocator(sizeof(*new_node));
	if (new_node == NULL) {
		return false;
	}
	new_node->data = value;
	new_node->next = NULL;

	if (is_empty(q)) {
		q->front = new_node;
	} else {
		q->rear->next = new_node;
	}
	q->rear = new_node;
	q->size++;
	return true;
}

void
clean_queue(struct queue *q, void (*deallocator)(void *))
{
	if (q == NULL)
		return;
	int ret;
	do {
		ret = dequeue(q, deallocator);
	} while (ret != -1);
}

int
dequeue(struct queue *q, void (*deallocator)(void *))
{
	if (is_empty_and_initialized(q) == QUEUE_UNINITIALIZED || q->front == NULL) {
		return -1;
	}

	struct queue_node *temp = q->front;
	int result = temp->data;

	q->front = q->front->next;
	if (q->front == NULL) {
		q->rear = NULL;
	}
	if (temp != NULL)
		deallocator(temp);
	q->size--;
	return result;
}

void
free_queue(struct queue *q, void (*deallocator)(void *))
{
	while (!is_empty(q)) {
		dequeue(q, deallocator);
	}
	deallocator(q);
}
