#include "errno.h"
#include "kernel_signal.h"
#include "proc.h"
#include "spinlock.h"
#include "file.h"
#include "pipe.h"
#include "kalloc.h"
#include <limits.h>
#include "lib/ring_buffer.h"

#define PIPESIZE PIPE_BUF


int
pipealloc(struct file **f0, struct file **f1)
{
	struct pipe *p;

	p = NULL;
	*f0 = *f1 = NULL;
	if ((*f0 = filealloc()) == NULL || (*f1 = filealloc()) == NULL)
		goto bad;
	if ((p = kmalloc(sizeof(*p))) == NULL)
		goto bad;
	if ((p->ring_buffer = ring_buffer_create(PIPESIZE, kmalloc)) == NULL)
		goto bad;
	// This creates a pipe like in pipe(2), but we reuse this
	// function to make FIFOs.
	p->readopen = 1;
	p->writeopen = 1;
	initlock(&p->lock, "pipe");
	(*f0)->type = FD_PIPE;
	(*f0)->readable = 1;
	(*f0)->writable = 0;

	// Notice here how f0 and f1 both assign the same pipe.
	// The pipe is aliased here.
	(*f0)->pipe = p;
	(*f1)->type = FD_PIPE;
	(*f1)->readable = 0;
	(*f1)->writable = 1;
	(*f1)->pipe = p;
	return 0;

bad:
	if (p) {
		ring_buffer_destroy(p->ring_buffer, kfree);
		kfree(p);
	}
	if (*f0)
		fileclose(*f0);
	if (*f1)
		fileclose(*f1);
	return -1;
}

void
pipeclose(struct pipe *p, int writable)
{
	acquire(&p->lock);
	if (writable) {
		p->writeopen--;
		wakeup(&p->ring_buffer->nread);
	} else {
		p->readopen--;
		wakeup(&p->ring_buffer->nwrite);
	}
	if (p->readopen == 0 && p->writeopen == 0) {
		release(&p->lock);
		ring_buffer_destroy(p->ring_buffer, kfree);
		kfree(p);
	} else {
		release(&p->lock);
	}
}
int
pipewrite(struct pipe *p, char *addr, int n)
{
	int i;

	acquire(&p->lock);

	// The pipe is not open for reading and
	// we are trying to write to it. (Broken pipe)
	if (p->readopen == 0 || myproc()->killed) {
		release(&p->lock);
		kill(myproc()->pid, SIGPIPE);
		return -EPIPE;
	}
	for (i = 0; i < n; i++) {
		while (p->ring_buffer->nwrite == p->ring_buffer->nread + p->ring_buffer->size) {
			if (p->readopen == 0 || myproc()->killed) {
				release(&p->lock);
				return -EPIPE;
			}
			wakeup(&p->ring_buffer->nread);
			sleep(&p->ring_buffer->nwrite, &p->lock);
		}
		p->ring_buffer->data[p->ring_buffer->nwrite++ % p->ring_buffer->size] = addr[i];
	}
	wakeup(&p->ring_buffer->nread);
	release(&p->lock);
	return n;
}

int
piperead(struct pipe *p, char *addr, int n)
{
	int i;

	acquire(&p->lock);
	 while (p->ring_buffer->nread == p->ring_buffer->nwrite && p->writeopen) {
		if (myproc()->killed) {
			release(&p->lock);
			return -1;
		}
		sleep(&p->ring_buffer->nread, &p->lock);
	}
	for (i = 0; i < n; i++) {
		if (p->ring_buffer->nread == p->ring_buffer->nwrite)
			break;
		addr[i] = p->ring_buffer->data[p->ring_buffer->nread++ % p->ring_buffer->size];
	}
	wakeup(&p->ring_buffer->nwrite);
	release(&p->lock);
	return i;
}
