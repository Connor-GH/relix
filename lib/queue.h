#pragma once
#include <stddef.h>
struct queue_node {
  int data;
  struct queue_node *next;
};

struct queue {
  struct queue_node *front;
  struct queue_node *rear;
  int size;
};

// Returns NULL on error.
struct queue *
create_queue(void *(*allocator)(size_t));

// Enqueues the element. Returns true on success,
// QUEUE_UNINITIALIZED on the queue being uninitialized,
// and false if we failed to allocate.
int
enqueue(struct queue *q, int value, void *(*allocator)(size_t), size_t limit);

// Dequeue an element. Returns NULL on error.
// Otherwise, returns a pointer to the element.
int
dequeue(struct queue *q, void (*deallocator)(void *));

void
free_queue(struct queue *q, void (*deallocator)(void *));

// Removes everything in the queue.
void
clean_queue(struct queue *q, void (*deallocator)(void *));
#define QUEUE_UNINITIALIZED (-1)
#define QUEUE_EMPTY (1)
#define QUEUE_POPULATED (0)
