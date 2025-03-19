#pragma once
#include <stdint.h>
#include "lib/queue.h"
void
kbdintr(void);
void
kbdinit(void);
int
kbd_scancode_into_char(uint32_t data);

int
kbd_enqueue(int value);
