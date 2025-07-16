#include "ticketlock.h"
#include "console.h"
#include "kernel_assert.h"
#include "proc.h"
#include "stddef.h"
#include <stdatomic.h>
#include <stdint.h>

int
holding_ticketlock(struct ticketlock *tl)
{
	return tl->next_ticket != tl->now_serving;
}

void
init_ticketlock(struct ticketlock *tl, const char *name)
{
	atomic_store(&tl->next_ticket, 0);
	atomic_store(&tl->now_serving, 0);
	tl->name = name;
	tl->cpu = 0;
}

void
acquire_ticketlock(struct ticketlock *tl)
{
	pushcli();
	if (holding_ticketlock(tl)) {
		uart_printf("%s\n", tl->name);
	}
	kernel_assert(!holding_ticketlock(tl));
	uint64_t my_ticket = atomic_fetch_add(&tl->next_ticket, 1);
	uint64_t now_serving = atomic_load(&tl->now_serving);
	while (now_serving != my_ticket) {
		now_serving = atomic_load(&tl->now_serving);
	}
	tl->cpu = mycpu();
	getcallerpcs(tl->pcs);
}

void
release_ticketlock(struct ticketlock *tl)
{
	kernel_assert(holding_ticketlock(tl));
	tl->pcs[0] = 0;
	tl->cpu = NULL;
	atomic_thread_fence(memory_order_release);
	atomic_fetch_add(&tl->now_serving, 1);
	popcli();
}
