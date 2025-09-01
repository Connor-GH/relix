#pragma once

#define BUTTON_LEFT 1 << 0
#define BUTTON_RIGHT 1 << 1
#define BUTTON_MIDDLE 1 << 2
#define MOUSE_XSIGN 1 << 4
#define MOUSE_YSIGN 1 << 5
#define MOUSE_ALWAYS_SET 0xC0

void ps2mouseintr(void);
void ps2mouseinit(void);
