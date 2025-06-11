#pragma once
#include "lib/queue.h"
#include <stdint.h>
void kbdintr(void);
void kbdinit(void);
int kbd_scancode_into_char(uint32_t data);

int kbd_enqueue(unsigned char value);
