#pragma once
#include "lib/queue.h"
#include <stdint.h>
void kbdintr(void);
void dev_kbd_init(void);
int kbd_scancode_into_char(uint32_t data);

int kbd_enqueue(unsigned char value);
