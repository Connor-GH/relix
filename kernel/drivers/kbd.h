#pragma once
#include <stdint.h>
#include "lib/queue.h"
void
kbdintr(void);
void
kbdinit(void);
int
kbd_scancode_into_char(uint32_t data);

extern struct queue *g_kbd_queue;
